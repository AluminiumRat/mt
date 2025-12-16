#include <stdexcept>

#include <resourceManagement/BufferResourceManager.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/ContentLoader.h>
#include <util/Log.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/DataBuffer.h>
#include <vkr/Device.h>

namespace fs = std::filesystem;

using namespace mt;

class BufferResourceManager::LoadingTask : public AsyncTask
{
public:
  LoadingTask(const fs::path& filePath,
                  CommandQueueTransfer& ownerQueue,
                  BufferResourceManager& manager) :
    AsyncTask((const char*)filePath.u8string().c_str(),
              AsyncTask::EXCLUSIVE_MODE,
              AsyncTask::SILENT),
    _filePath(filePath),
    _ownerQueue(ownerQueue),
    _manager(manager)
  {
  }

  virtual void asyncPart() override
  {
    AsyncTask::asyncPart();

    std::string fileName = (const char*)_filePath.u8string().c_str();

    std::vector<char> fileData = ContentLoader::getLoader().loadData(_filePath);
    if(fileData.empty()) throw std::runtime_error(std::string("BufferResourceManager: ") + fileName + " is empty");

    Ref<DataBuffer> buffer(new DataBuffer(_ownerQueue.device(),
                                          fileData.size(),
                                          DataBuffer::STORAGE_BUFFER,
                                          fileName.c_str()));
    _ownerQueue.uploadToBuffer(*buffer, 0, fileData.size(), fileData.data());

    _loadedBuffer = buffer;
  }

  virtual void finalizePart() override
  {
    MT_ASSERT(_loadedBuffer != nullptr);
    AsyncTask::finalizePart();
    _manager._updateResource(_filePath, _ownerQueue, *_loadedBuffer);
  }

private:
  fs::path _filePath;
  CommandQueueTransfer& _ownerQueue;
  BufferResourceManager& _manager;
  ConstRef<DataBuffer> _loadedBuffer;
};

BufferResourceManager::BufferResourceManager( FileWatcher& fileWatcher,
                                              AsyncTaskQueue& loadingQueue) :
  _fileWatcher(fileWatcher),
  _loadingQueue(loadingQueue)
{
}

BufferResourceManager::~BufferResourceManager() noexcept
{
  _fileWatcher.removeObserver(*this);
}

BufferResourceManager::ResourceRecord*
            BufferResourceManager::_getExistingResource(
                                              const fs::path& filePath,
                                              CommandQueueTransfer& ownerQueue)
{
  ResourcesMap::iterator iResource = _resources.find({filePath, &ownerQueue});
  if(iResource == _resources.end()) return nullptr;
  return &iResource->second;
}

static Ref<DataBuffer> loadData(const fs::path& filePath,
                                CommandQueueTransfer& ownerQueue)
{
  try
  {
    std::string fileName = (const char*)filePath.u8string().c_str();

    std::vector<char> fileData = ContentLoader::getLoader().loadData(filePath);
    if(fileData.empty()) throw std::runtime_error(std::string("BufferResourceManager: ") + fileName + " is empty");

    Ref<DataBuffer> buffer(new DataBuffer(ownerQueue.device(),
                                          fileData.size(),
                                          DataBuffer::STORAGE_BUFFER,
                                          fileName.c_str()));
    ownerQueue.uploadToBuffer(*buffer, 0, fileData.size(), fileData.data());
    return buffer;
  }
  catch(std::exception& error)
  {
    Log::error() << "BufferResourceManager: unable to load: " << filePath << " : " << error.what();
    return Ref<DataBuffer>();
  }
}

BufferResourceManager::ResourceRecord&
                      BufferResourceManager::_createNewRecord(
                                                const fs::path& filePath,
                                                CommandQueueTransfer& ownerQueue,
                                                const DataBuffer* buffer)
{
  ResourceRecord newRecord;
  newRecord.waitableResource = Ref(new TechniqueResource);
  newRecord.immediatellyResource = Ref(new TechniqueResource);

  if (buffer != nullptr)
  {
    newRecord.waitableResource->setBuffer(buffer);
    newRecord.immediatellyResource->setBuffer(buffer);
  }

  _resources[{filePath, &ownerQueue}] = std::move(newRecord);

  return _resources[{filePath, &ownerQueue}];
}

ConstRef<TechniqueResource> BufferResourceManager::loadImmediately(
                                              const fs::path& filePath,
                                              CommandQueueTransfer& ownerQueue)
{
  fs::path normalizedPath = filePath.lexically_normal();

  {
    // Первичная проверка, не загружен ли уже ресурс
    std::lock_guard lock(_accessMutex);
    ResourceRecord* record = _getExistingResource( normalizedPath, ownerQueue);
    if(record != nullptr && record->immediatellyResource != nullptr)
    {
      return record->immediatellyResource;
    }
  }

  //  Если ресурс ещё не был загружен, загружаем его
  Ref<DataBuffer> buffer = loadData(normalizedPath, ownerQueue);

  //  Повторная проверка, может кто-то ещё успел загрузить ресурс пока мы читали
  //  его с диска
  std::lock_guard lock(_accessMutex);
  ResourceRecord* record = _getExistingResource( normalizedPath, ownerQueue);
  if(record != nullptr && record->immediatellyResource != nullptr)
  {
    return record->immediatellyResource;
  }

  if(record == nullptr)
  {
    record = &_createNewRecord( normalizedPath, ownerQueue, buffer.get());
  }
  else
  {
    record->immediatellyResource = Ref(new TechniqueResource);
    record->immediatellyResource->setBuffer(buffer.get());
  }

  _addFileWatching(normalizedPath);

  return record->immediatellyResource;
}

void BufferResourceManager::_addFileWatching(const fs::path& filePath) noexcept
{
  try
  {
    _fileWatcher.addWatching(filePath, *this);
  }
  catch(std::exception& error)
  {
    Log::error() << "BufferResourceManager: unable to add file for watching: " << filePath << " : " << error.what();
  }
}

ConstRef<TechniqueResource> BufferResourceManager::scheduleLoading(
                                          const std::filesystem::path& filePath,
                                          CommandQueueTransfer& ownerQueue)
{
  fs::path normalizedPath = filePath.lexically_normal();

  std::lock_guard lock(_accessMutex);

  //  Проверка, не создан ли уже ресурс
  ResourcesMap::const_iterator iResource =
                              _resources.find({ normalizedPath, &ownerQueue });
  if(iResource != _resources.end())
  {
    MT_ASSERT(iResource->second.waitableResource != nullptr);
    return iResource->second.waitableResource;
  }

  //  Если ресурс ещё не был создан, то создаем новую запись в таблице
  //  ресурсов и отправляем задачу на загрузку
  ResourceRecord newRecord;
  newRecord.waitableResource = Ref(new TechniqueResource);

  std::unique_ptr<LoadingTask> loadTask(new LoadingTask(normalizedPath,
                                                        ownerQueue,
                                                        *this));
  if(newRecord.loadingHandle != nullptr) newRecord.loadingHandle->abortTask();
  newRecord.loadingHandle = _loadingQueue.addManagedTask(std::move(loadTask));

  _resources[{normalizedPath, & ownerQueue}] = std::move(newRecord);

  _addFileWatching(normalizedPath);

  return _resources[{normalizedPath, & ownerQueue}].waitableResource;
}

void BufferResourceManager::onFileChanged(const fs::path& filePath,
                                          EventType eventType)
{
  if(eventType == FILE_DISAPPEARANCE) return;

  std::lock_guard lock(_accessMutex);

  //  Если файл изменился или вновь появился, то запускаем его перезагрузку
  //  в фоновом режиме
  for(ResourcesMap::iterator iResource = _resources.begin();
      iResource != _resources.end();
      iResource++)
  {
    const ResourcesKey& key = iResource->first;
    if(key.filePath == filePath)
    {
      ResourceRecord& resource = iResource->second;
      std::unique_ptr<LoadingTask> loadingTask(new LoadingTask(
                                                              filePath,
                                                              *key.commandQueue,
                                                              *this));
      if(resource.loadingHandle != nullptr) resource.loadingHandle->abortTask();
      resource.loadingHandle =
                          _loadingQueue.addManagedTask(std::move(loadingTask));
    }
  }
}

void BufferResourceManager::_updateResource(const fs::path& filePath,
                                            CommandQueueTransfer& ownerQueue,
                                            const DataBuffer& buffer)
{
  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource = _resources.find({filePath, &ownerQueue});
  MT_ASSERT(iResource != _resources.end());

  ResourceRecord& resource = iResource->second;

  MT_ASSERT(resource.waitableResource != nullptr);
  resource.waitableResource->setBuffer(&buffer);

  if (resource.immediatellyResource == nullptr)
  {
    resource.immediatellyResource = Ref(new TechniqueResource);
  }
  resource.immediatellyResource->setBuffer(&buffer);
}

int BufferResourceManager::_getRecordsCount(
                                        const fs::path& filePath) const noexcept
{
  int count = 0;
  for(ResourcesMap::const_iterator iResource = _resources.begin();
      iResource != _resources.end();
      iResource++)
  {
    if(iResource->first.filePath == filePath) count++;
  }

  return count;
}

void BufferResourceManager::removeUnused() noexcept
{
  std::lock_guard lock(_accessMutex);

  ResourcesMap::iterator iResource = _resources.begin();
  while(iResource != _resources.end())
  {
    const ResourcesKey& key = iResource->first;
    ResourceRecord& resource = iResource->second;
    MT_ASSERT(resource.waitableResource != nullptr);

    if( resource.waitableResource->counterValue() == 1 &&
        (resource.immediatellyResource == nullptr ||
          resource.immediatellyResource->counterValue() == 1))
    {
      //  Ищем другие ресурсы, ссылающиеся на тот же файл, чтобы понять, надо ли
      //  прекращать слежение за файлом
      if (_getRecordsCount(key.filePath) == 1)
      {
        _fileWatcher.removeWatching(key.filePath, *this);
      }
      iResource = _resources.erase(iResource);
    }
    else iResource++;
  }
}
