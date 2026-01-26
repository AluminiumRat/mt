#include <stdexcept>

#include <resourceManagement/TechniqueManager.h>
#include <technique/TechniqueLoader.h>
#include <util/Assert.h>
#include <util/Log.h>

namespace fs = std::filesystem;

using namespace mt;

class TechniqueManager::RebuildTask : public AsyncTask
{
public:
  RebuildTask(TechniqueManager& manager,
              TechniqueManager::TechniqueResource& resource,
              const fs::path& filePath) :
    AsyncTask((const char*)filePath.u8string().c_str(),
              AsyncTask::EXCLUSIVE_MODE,
              AsyncTask::SILENT),
    _manager(manager),
    _resource(resource),
    _filePath(filePath)
  {
  }

  virtual void asyncPart() override
  {
    AsyncTask::asyncPart();
    _resource.rebuildConfiguration();
  }

  virtual void finalizePart() override
  {
    AsyncTask::finalizePart();
    _resource.propagateConfiguration();
  }

private:
  TechniqueManager& _manager;
  TechniqueManager::TechniqueResource& _resource;
  fs::path _filePath;
};

TechniqueManager::TechniqueResource::TechniqueResource(
                                                    const fs::path& filePath,
                                                    Device& device,
                                                    TechniqueManager& manager) :
  _manager(manager),
  _filePath(filePath),
  _configurator(new TechniqueConfigurator(
                                    device,
                                    (const char*)filePath.u8string().c_str())),
  _processed(false)
{
}

TechniqueManager::TechniqueResource::~TechniqueResource() noexcept
{
  _manager._fileWatcher.removeObserver(*this);
}

void TechniqueManager::TechniqueResource::rebuildConfiguration()
{
  std::lock_guard lock(_configuratorMutex);
  _rebuild();
}

void TechniqueManager::TechniqueResource::_rebuild()
{
  _processed = true;

  std::unordered_set<fs::path> usedFiles;
  try
  {
    loadConfigurator(*_configurator, _filePath, &usedFiles);
    _configurator->rebuildOnlyConfiguration(&usedFiles);
  }
  catch(...)
  {
    _updateFileWatching(std::move(usedFiles));
    throw;
  }
  _updateFileWatching(std::move(usedFiles));
}

void TechniqueManager::TechniqueResource::_updateFileWatching(
                                std::unordered_set<fs::path>&& newList) noexcept
{
  try
  {
    //  Добавляем файлы, которых раньше не было
    for(const fs::path& newFile: newList)
    {
      if(_watchedFiles.count(newFile) == 0)
      {
        _manager._fileWatcher.addWatching(newFile, *this);
      }
    }

    //  Убираем файлы, которые были, но пропали из техники
    for(const fs::path& oldFile : _watchedFiles)
    {
      if(newList.count(oldFile) == 0)
      {
        _manager._fileWatcher.removeWatching(oldFile, *this);
      }
    }

    _watchedFiles.swap(newList);
  }
  catch(std::exception& error)
  {
    Log::error() << "TechniqueManager: unable to update files watching: " << _filePath << " : " << error.what();
    _watchedFiles.clear();
    _manager._fileWatcher.removeObserver(*this);
  }
}

std::unique_ptr<Technique>
                TechniqueManager::TechniqueResource::createTechnique(
                                                            bool checkProcessed)
{
  std::lock_guard lock(_configuratorMutex);
  if(checkProcessed && !_processed) _rebuild();
  return std::unique_ptr<Technique>(new Technique(*_configurator));
}

void TechniqueManager::TechniqueResource::propagateConfiguration()
{
  std::lock_guard lock(_configuratorMutex);
  _configurator->propogateConfiguration();
}

bool TechniqueManager::TechniqueResource::used() const noexcept
{
  return _configurator->counterValue() != 1;
}

void TechniqueManager::TechniqueResource::scheduleRebuild()
{
  std::unique_ptr<RebuildTask> rebuildTask( new RebuildTask(_manager,
                                                            *this,
                                                            _filePath));
  if(_loadingHandle != nullptr) _loadingHandle->abortTask();
  _loadingHandle =
                  _manager._loadingQueue.addManagedTask(std::move(rebuildTask));
}

void TechniqueManager::TechniqueResource::onFileChanged(const fs::path&,
                                                        EventType eventType)
{
  if (eventType == FILE_DISAPPEARANCE) return;
  std::lock_guard lock(_manager._accessMutex);
  scheduleRebuild();
}

TechniqueManager::TechniqueManager( FileWatcher& fileWatcher,
                                    AsyncTaskQueue& loadingQueue) :
  _fileWatcher(fileWatcher),
  _loadingQueue(loadingQueue)
{
}

std::unique_ptr<Technique> TechniqueManager::loadImmediately(
                                                        const fs::path& filePath,
                                                        Device& device)
{
  fs::path normalizedPath = filePath.lexically_normal();

  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource = _resources.find({normalizedPath, &device});
  if(iResource != _resources.end())
  {
    return iResource->second->createTechnique(true);
  }

  std::unique_ptr<TechniqueResource> newResource(
                          new TechniqueResource(normalizedPath, device, *this));
  try
  {
    newResource->rebuildConfiguration();
  }
  catch(std::exception& error)
  {
    Log::error() << "TechniqueManager::unable to load technique " << filePath << " : " << error.what();
  }

  std::unique_ptr<Technique> technique = newResource->createTechnique(true);

  _resources[{normalizedPath, & device}] = std::move(newResource);

  return technique;
}

std::unique_ptr<Technique> TechniqueManager::scheduleLoading(
                                          const std::filesystem::path& filePath,
                                          Device& device)
{
  fs::path normalizedPath = filePath.lexically_normal();

  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource = _resources.find({normalizedPath, &device});
  if(iResource != _resources.end())
  {
    return iResource->second->createTechnique(false);
  }

  std::unique_ptr<TechniqueResource> newResource(
                          new TechniqueResource(normalizedPath, device, *this));
  newResource->scheduleRebuild();

  std::unique_ptr<Technique> technique = newResource->createTechnique(false);

  _resources[{normalizedPath, & device}] = std::move(newResource);

  return technique;
}

void TechniqueManager::removeUnused() noexcept
{
  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource = _resources.begin();
  while(iResource != _resources.end())
  {
    if(iResource->second->used()) iResource++;
    else iResource = _resources.erase(iResource);
  }
}
