#include <algorithm>
#include <stdexcept>

#include <resourceManagement/FileWatcher.h>
#include <util/ContentLoader.h>
#include <util/Log.h>

namespace fs = std::filesystem;

using namespace mt;

using Filetime = fs::file_time_type;

FileWatcher* FileWatcher::_instance = nullptr;

//  Нода в дереве обозреваемых каталогов
class FileWatcher::ObservedFile
{
public:
  //  Полный путь до файла/каталога (может быть и абсолютным и относительным)
  fs::path filePath;
  //  Список обсерверов, которые отслеживают состояние этого файла
  std::vector<FileObserver*> observers;
  //  Время последней известной модификации. По нему определяем, что произошли
  //  изменения.
  Filetime modificationTime;
  //  Существовал ли файл на момент последней проверки
  bool exists = false;

  //  Добавить в очередь events события для всех связанных обсерверов
  //  Вспомогательный метод, вызывается при обновлении.
  void addEvents(EventQueue& events, FileObserver::EventType type)
  {
    size_t oldQueueSize = events.size();
    try
    {
      for (FileObserver* observer : observers)
      {
        events.emplace_back(observer, filePath, type);
      }
    }
    catch(...)
    {
      //  В случае исключения откатим очередь событий в исходное состояние,
      //  чтобы на следующем апдэйте не получить дубликаты
      while(events.size() != oldQueueSize) events.pop_back();
      throw;
    }
  }

  void update(EventQueue& events)
  {
    if(!ContentLoader::getLoader().exists(filePath))
    {
      if(exists)
      {
        addEvents(events, FileObserver::FILE_DISAPPEARANCE);
        exists = false;
      }
    }
    else
    {
      Filetime lastWriteTime =
                            ContentLoader::getLoader().lastWriteTime(filePath);
      if (!exists)
      {
        addEvents(events, FileObserver::FILE_APPEARANCE);
        exists = true;
        modificationTime = lastWriteTime;
      }

      //  Проверяем, были ли изменения в файловой системе
      if(modificationTime != lastWriteTime)
      {
        addEvents(events, FileObserver::FILE_CHANGE);
        modificationTime = lastWriteTime;
      }
    }
  }
};

FileWatcher::FileWatcher()  :
  _stopUpdate(false)
{
  MT_ASSERT(_instance == nullptr);
  _instance = this;
  _updateThread = std::thread(&FileWatcher::_updateCycle, this);
}

FileWatcher::~FileWatcher() noexcept
{
  _stopUpdate = true;
  _updateThread.join();

  MT_ASSERT(_instance != nullptr);
  MT_ASSERT(_observers.empty());
  MT_ASSERT(_files.empty());
  _instance = nullptr;
}

void FileWatcher::_updateCycle()
{
  size_t lastProcessedFile = 0;

  while(!_stopUpdate)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::unique_lock lock(_dataMutex);
    try
    {
      size_t filesToCheck = std::min(_files.size(), (size_t)10);
      for(; filesToCheck != 0; filesToCheck--)
      {
        lastProcessedFile = (lastProcessedFile + 1) % _files.size();
        _files[lastProcessedFile]->update(_eventQueue);
      }
    }
    catch(std::exception& error)
    {
      Log::warning() << "FileWatcher::update: " << error.what();
    }
  }
}

FileWatcher::ObservedFile* FileWatcher::_getFileRecord(
                                              const fs::path& filePath) noexcept
{
  for (std::unique_ptr<ObservedFile>& record : _files)
  {
    if (record->filePath == filePath) return record.get();
  }
  return nullptr;
}

void FileWatcher::_addToFileTable(FileObserver& observer,
                                  const fs::path& filePath)
{
  ObservedFile* fileRecord = _getFileRecord(filePath);
  if (fileRecord == nullptr)
  {
    std::unique_ptr<ObservedFile> newRecord(new ObservedFile);
    newRecord->filePath = filePath;

    std::error_code errCode;
    try
    {
      newRecord->exists = ContentLoader::getLoader().exists(filePath);
      if (newRecord->exists)
      {
        newRecord->modificationTime =
                  ContentLoader::getLoader().lastWriteTime(newRecord->filePath);
      }
    }
    catch(...)
    {
      newRecord->exists = false;
    }

    newRecord->observers.push_back(&observer);
    _files.push_back(std::move(newRecord));
  }
  else
  {
    fileRecord->observers.push_back(&observer);
  }
}

bool FileWatcher::addWatching(const fs::path& filePatch,
                              FileObserver& observer)
{
  fs::path normalizedPath = filePatch.lexically_normal();
  MT_ASSERT(!normalizedPath.empty());

  std::lock_guard lock(_dataMutex);

  //  Проверяем, не регистрировалась ли уже эта пара
  FileList& files = _observers[&observer];
  for(const fs::path& observedFile : files)
  {
    if(normalizedPath == observedFile) return false;
  }

  files.push_back(normalizedPath);

  try
  {
    _addToFileTable(observer, normalizedPath);
  }
  catch(...)
  {
    files.pop_back();
    throw;
  }

  return true;
}

void FileWatcher::_removeFromFileTable( const FileObserver& observer,
                                        const fs::path& filePath) noexcept
{
  for(FilesTable::iterator iFile = _files.begin();
      iFile != _files.end();
      iFile++)
  {
    ObservedFile* fileRecord = iFile->get();
    if(fileRecord->filePath == filePath)
    {
      std::vector<FileObserver*>::iterator iObserver =
                                      std::find(fileRecord->observers.begin(),
                                                fileRecord->observers.end(),
                                                &observer);
      MT_ASSERT(iObserver != fileRecord->observers.end());
      fileRecord->observers.erase(iObserver);

      if(fileRecord->observers.empty()) _files.erase(iFile);

      break;
    }
  }
}

bool FileWatcher::removeWatching( const fs::path& filePatch,
                                  const FileObserver& observer) noexcept
{
  fs::path normalizedPath = filePatch.lexically_normal();
  MT_ASSERT(!normalizedPath.empty());

  std::lock_guard lock(_dataMutex);

  Observers::iterator observerRecord = _observers.find(&observer);
  if(observerRecord == _observers.end()) return false;

  // Ищем и удаляем файл из списка файлов обсервера
  bool fileIsFound = false;
  for(FileList::iterator iFile = observerRecord->second.begin();
      iFile != observerRecord->second.end();
      iFile++)
  {
    if(normalizedPath == *iFile)
    {
      observerRecord->second.erase(iFile);
      fileIsFound = true;
      break;
    }
  }

  if(fileIsFound)
  {
    //  Удаляем пару обсерве-файл из очереди событий. Делаем через пересоздание
    //  очереди
    EventQueue newQueue;
    for(const Event& theEvent : _eventQueue)
    {
      if(theEvent.file != normalizedPath || theEvent.observer != &observer)
      {
        newQueue.push_back(theEvent);
      }
    }
    _eventQueue = newQueue;

    //  Если у обсервера нет больше файлов, за которыми он смотрит, то удаляем
    //  обсервер из списка зарегестрированных обсерверов
    if(observerRecord->second.empty()) _observers.erase(observerRecord);

    _removeFromFileTable(observer, normalizedPath);
  }

  return fileIsFound;
}

bool FileWatcher::removeObserver(const FileObserver& observer) noexcept
{
  std::lock_guard lock(_dataMutex);

  Observers::iterator observerRecord = _observers.find(&observer);
  if (observerRecord == _observers.end()) return false;

  //  Удаляем из списка файлов все файлы обсервера
  for(const fs::path& file : observerRecord->second)
  {
    _removeFromFileTable(*observerRecord->first, file);
  }

  //  Удаляем все события с этим обсервером
  EventQueue newQueue;
  for (const Event& theEvent : _eventQueue)
  {
    if (theEvent.observer != &observer)
    {
      newQueue.push_back(theEvent);
    }
  }
  _eventQueue = newQueue;

  //  Удаляем сам обссервер из списка
  _observers.erase(observerRecord);
  return true;
}

void FileWatcher::propagateChanges()
{
  std::lock_guard lock(_dataMutex);

  while(!_eventQueue.empty())
  {
    Event theEvent = _eventQueue.back();
    _eventQueue.pop_back();
    try
    {
      theEvent.observer->onFileChanged(theEvent.file, theEvent.type);
    }
    catch(std::exception& error)
    {
      Log::warning() << "FileWatcher::update: observer exception: " << error.what();
    }
  }
}
