#pragma once

#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <util/Assert.h>

namespace mt
{
  //  Интефейс объекта, который следит за каким-то файлом
  //  Классы, которым необходимо отслеживать состояние каких-либо файлов должны
  //    унаследоваться от этого интефейса и добавить его в FileWatcher
  //  ВНИМАНИЕ! Не забудьте удалить обсервер из FileWatcher-а до уничтожения
  //    обсервера
  class FileObserver
  {
  public:
    //  Набор отслеживаемых событий, которые могут случиться с файлом
    enum EventType
    {
      FILE_APPEARANCE,      //  До события файла не существовало, после события
                            //    он появился (создан или скопирован откуда-то)
      FILE_DISAPPEARANCE,   //  До события файл существовал, после события
                            //    он исчер (удален или переименован)
      FILE_CHANGE           //  Изменилось содержимое файла. Если файл - это
                            //    каталог, то изменился зотя-бы один из его
                            //    дочерних фалов или их состав
    };

  public:
    virtual void onFileChanged( const std::filesystem::path& filePatch,
                                EventType eventType) = 0;
  };

  //  Класс осуществляющий централизованное слежение за состоянием большого
  //    количества файлов.
  //  Синглтон.
  //  Объект класса должен быть явно создан через конструктор и существовать
  //    на протяжении всего времени, пока существует хотябы один FileObserver
  //  Доступ к объекту можно производить через метод instance, но только после
  //    явного создания объекта
  //  Сам по себе класс полностью потокобезопасный, однако FileObserver-ы не
  //    обязаны быть потокобезопасными. Поэтому метод update должен иметь
  //    внешнюю синхронизацию (лучше используйте в основном потоке с синхронной
  //    части основного цикла)
  class FileWatcher
  {
  public:
    FileWatcher();
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator = (const FileWatcher&) = delete;
    ~FileWatcher() noexcept;

    //  Нельзя вызывать до создания FilesWatcher
    inline static FileWatcher& instance() noexcept;

    //  Добавить слежение за файлом
    //  Возвращает false, если эта пара обсервер/файл уже обрабатывается
    bool addWatching( const std::filesystem::path& filePatch,
                      FileObserver& observer);
    //  Прекратить посылать обсерверу сообщения об изменениях в файле
    //  Когда не останется файлов, за которыми следит observer, обсервер
    //    автоматически удалится из вотчера
    //  Возвращает false, если эта пара обсервер/файл не обрабатывается
    bool removeWatching(const std::filesystem::path& filePatch,
                        const FileObserver& observer) noexcept;
    //  Полностью прекратить послылать обсерверу сообщения и удалить его из
    //    вотчера
    //  Возвращает false, если этот обсервер не зарегистрирован
    bool removeObserver(const FileObserver& observer) noexcept;

    //  Проверить, были ли изменения в отслеживаемых файлах и раздать
    //    сообщения об изменениях обсерверам.
    //  Внимание! Обсерверы не обязаны быть потокобезопасными, поэтому
    //    вызывайте этот метод в безопасном месте.
    void update();

  private:
    //  Таблица наблюдаемых файлов
    class ObservedFile;
    using FilesTable = std::vector<std::unique_ptr<ObservedFile>>;

    struct Event
    {
      FileObserver* observer;
      std::filesystem::path file;
      FileObserver::EventType type;
    };

  private:
    ObservedFile* _getFileRecord(
                                const std::filesystem::path& filePath) noexcept;
    void _addToFileTable( FileObserver& observer,
                          const std::filesystem::path& filePath);
    void _removeFromFileTable( const FileObserver& observer,
                               const std::filesystem::path& filePath) noexcept;

  private:
    static FileWatcher* _instance;

    using FileList = std::vector<std::filesystem::path>;
    using Observers = std::unordered_map<const FileObserver*, FileList>;
    Observers _observers;

    FilesTable _files;

    using EventQueue = std::deque<Event>;
    EventQueue _eventQueue;

    mutable std::recursive_mutex _mutex;
  };

  inline FileWatcher& FileWatcher::instance() noexcept
  {
    MT_ASSERT(_instance != nullptr);
    return *_instance;
  }
}