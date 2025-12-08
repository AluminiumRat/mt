#include <algorithm>
#include <stdexcept>

#include <resourceManagement/FileWatcher.h>
#include <util/Log.h>

namespace fs = std::filesystem;

using namespace mt;

using Filetime = fs::file_time_type;

FileWatcher* FileWatcher::_instance = nullptr;

//  Нода в дереве обозреваемых каталогов
class FileWatcher::PathTreeNode
{
public:
  //  Имя без пути
  fs::path name;
  //  Полный путь до файла/каталога (может быть и абсолютным и относительным)
  fs::path fullPath;
  //  Сколько FileObserver-ов отслеживают состояние этого файла(каталога) и
  //    файлов, которые находятся внутри(если это каталог)
  size_t usingCounter = 0;
  //  Если нода указывает на каталог, то здесь находится список дочерних файлов
  //    или подкаталогов которые также отслеживаются
  PathNodes childs;
  //  Список обсерверов, которые отслеживают состояние именно этого файла
  //  То есть для каталогов сюда не попадают обсерверы, которые следят за
  //    дочерними файлами
  std::vector<FileObserver*> observers;
  //  Время последней известной модификации файла/каталога. По нему определяем,
  //    что произошли изменения.
  Filetime modificationTime;
  //  Существовал ли файл на момент последней проверки
  bool exists = false;

  PathTreeNode* getDirectChild(const fs::path& name) noexcept
  {
    for(std::unique_ptr<PathTreeNode>& child : childs)
    {
      if(child->name == name) return child.get();
    }
    return nullptr;
  }

  // Рекурсивная вставка в дерево
  void insert(FileObserver& observer,
              const fs::path& filePath,
              fs::path::iterator pathBegin,   // Итераторы для рекурсивного
              fs::path::iterator pathEnd)     //  обхода
  {
    if(pathBegin == pathEnd)
    {
      // Мы дошли до конечной части в пути, добавим нового обсервера
      observers.push_back(&observer);
    }
    else
    {
      // Эта нода - только каталог, в котором находится файл, идем глубже
      PathTreeNode* childNode = getDirectChild(*pathBegin);
      if(childNode != nullptr)
      {
        childNode->insert(observer, filePath, ++pathBegin, pathEnd);
      }
      else
      {
        std::unique_ptr<PathTreeNode> newNode(new PathTreeNode);
        newNode->name = *pathBegin;
        newNode->fullPath = fullPath.empty() ?  *pathBegin :
                                                fullPath / *pathBegin;

        if(exists)
        {
          //  Если родительский каталог существует, то получим информацию
          //  об отслеживаемом файле/каталоге
          fs::directory_entry entry(newNode->fullPath);
          std::error_code errCode;
          newNode->exists = entry.exists(errCode);
          if(newNode->exists && !errCode)
          {
            newNode->modificationTime = entry.last_write_time(errCode);
          }
          newNode->exists &= !errCode;
        }

        newNode->insert(observer, filePath, ++pathBegin, pathEnd);
        childs.push_back(std::move(newNode));
      }
    }

    usingCounter++;
  }

  // Рекурсивное удаление из дерева
  void remove(const FileObserver& observer,
              fs::path::iterator pathBegin,   // Итераторы для рекурсивного
              fs::path::iterator pathEnd) noexcept
  {
    if (pathBegin == pathEnd)
    {
      //  Это - конечная нода, она указывает на нужный файл. Просто удаляем
      //  обсервер из списка
      std::vector<FileObserver*>::iterator iObserver =
                      std::find(observers.begin(), observers.end(), &observer);
      MT_ASSERT(iObserver != observers.end());
      observers.erase(iObserver);
    }
    else
    {
      //  Это - каталог, в котором лежит нужный файл, ищем нужную ноду глубже
      PathTreeNode* childNode = getDirectChild(*pathBegin);
      MT_ASSERT(childNode != nullptr);
      childNode->remove(observer, ++pathBegin, pathEnd);

      // Если никто не следит за потомком, то его лучше удалить
      if(childNode->usingCounter == 0)
      {
        for(PathNodes::iterator iChild = childs.begin();
            iChild != childs.end();
            iChild++)
        {
          if(childNode  == iChild->get())
          {
            childs.erase(iChild);
            break;
          }
        }
      }
    }

    MT_ASSERT(usingCounter != 0)
    usingCounter--;
  }

  //  Добавить в очередь events события для всех обсерверов, привязаных к
  //  ноде. Вспомогательный метод, вызывается при обновлении.
  void addEvents(EventQueue& events, FileObserver::EventType type)
  {
    size_t oldQueueSize = events.size();
    try
    {
      for (FileObserver* observer : observers)
      {
        events.emplace_back(observer, fullPath, type);
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

  //  Файл этой ноды или какой-то каталог, в котором файл находится, недоступен.
  //  Сформировать события для этой ноды и её потомков.
  //  Вспомогательный метод, вызывается при обновлении
  void propogateDisappear(EventQueue& events)
  {
    if(!exists) return;

    // Сначала обойдем потомков, чтобы они добавили сообщения первыми
    for (std::unique_ptr<PathTreeNode>& child : childs)
    {
      child->propogateDisappear(events);
    }

    // Добавим сообщения про себя
    addEvents(events, FileObserver::FILE_DISAPPEARANCE);
    exists = false;
  }

  //  Найти в дереве изменения с последнего update и поместить сообщения о них
  //  в events
  void update(EventQueue& events)
  {
    if( name.empty()/* ||
        fullPath.root_path() == fullPath ||
        fullPath.root_name() == fullPath*/)
    {
      // Это рутовая нода, просто транслируем команду потомкам
      for(std::unique_ptr<PathTreeNode>& child : childs) child->update(events);
      return;
    }

    fs::directory_entry entry(fullPath);
    entry.refresh();
    if(!entry.exists())
    {
      propogateDisappear(events);
    }
    else
    {
      bool needUpdateChilds = false;

      Filetime lastWriteTime = entry.last_write_time();
      //  Добавляем сообщение, о том, что файл только что появился.
      //  Это надо сделать до того как такие же сообщения будут добавлены
      //  потомками
      if (!exists)
      {
        addEvents(events, FileObserver::FILE_APPEARANCE);
        exists = true;
        modificationTime = lastWriteTime;
        needUpdateChilds = true;
      }

      //  Проверяем, были ли изменения в файловой системе
      if(modificationTime != lastWriteTime)
      {
        addEvents(events, FileObserver::FILE_CHANGE);
        modificationTime = lastWriteTime;
        needUpdateChilds = true;
      }

      if(needUpdateChilds)
      {
        for (std::unique_ptr<PathTreeNode>& child : childs)
        {
          child->update(events);
        }
      }
    }
  }
};

FileWatcher::FileWatcher() :
  _pathTree(new PathTreeNode)
{
  MT_ASSERT(_instance == nullptr);
  _instance = this;
  _pathTree->exists = true;
}

FileWatcher::~FileWatcher() noexcept
{
  MT_ASSERT(_instance != nullptr);
  MT_ASSERT(_observers.empty());
  MT_ASSERT(_pathTree->childs.empty());
  _instance = nullptr;
}

bool FileWatcher::addWatching(const fs::path& filePatch,
                              FileObserver& observer)
{
  fs::path normalizedPath = filePatch.lexically_normal();
  MT_ASSERT(!normalizedPath.empty());

  std::lock_guard lock(_mutex);

  //  Проверяем, не регистрировалась ли уже эта пара
  FileList& files = _observers[&observer];
  for(const fs::path& observedFile : files)
  {
    if(normalizedPath == observedFile) return false;
  }

  files.push_back(normalizedPath);

  try
  {
    _pathTree->insert(observer,
                      normalizedPath,
                      normalizedPath.begin(),
                      normalizedPath.end());
  }
  catch(...)
  {
    files.pop_back();
    throw;
  }

  return true;
}

bool FileWatcher::removeWatching( const fs::path& filePatch,
                                  const FileObserver& observer) noexcept
{
  fs::path normalizedPath = filePatch.lexically_normal();
  MT_ASSERT(!normalizedPath.empty());

  std::lock_guard lock(_mutex);

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
    _pathTree->remove(observer, normalizedPath.begin(), normalizedPath.end());
  }
  return fileIsFound;
}

bool FileWatcher::removeObserver(const FileObserver& observer) noexcept
{
  std::lock_guard lock(_mutex);

  Observers::iterator observerRecord = _observers.find(&observer);
  if (observerRecord == _observers.end()) return false;

  //  Удаляем из дерева каталогов все файлы обсервера
  for(const fs::path& file : observerRecord->second)
  {
    _pathTree->remove(*observerRecord->first, file.begin(), file.end());
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

void FileWatcher::update()
{
  // Для начала пополняем очередь событий
  try
  {
    _pathTree->update(_eventQueue);
  }
  catch(std::exception& error)
  {
    Log::warning() << "FileWatcher::update: " << error.what();
  }

  // Раздаем события обсерверам
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
