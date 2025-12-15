#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/TechniqueResource.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandQueueTransfer;
  class Device;

  //  Класс используется для централизованной загрузки данных для DataBuffer с
  //    диска и слежением за изменениями на диске. Также позволяет расшаривать
  //    буферы между многими потребителями, избега дублирования данных.
  //  Все методы класса потокобезопасные. Однако учтите, что обновление
  //    ресурсов (то есть передача им новых ImageView) производится в синхронной
  //    части AsyncTaskQueue
  class BufferResourceManager : private FileObserver
  {
  public:
    BufferResourceManager(FileWatcher& fileWatcher,
                          AsyncTaskQueue& loadingQueue);
    BufferResourceManager(const BufferResourceManager&) = delete;
    BufferResourceManager& operator = (const BufferResourceManager&) = delete;
    virtual ~BufferResourceManager() noexcept;

    //  Попытаться загрузить данные немедленно. Может вернуть пустой ресурс
    //    если не удается загрузить данные (нет файла, например).
    //  Независимо от того, будут ли загружены данные, файл будет направлен
    //    на отслеживание и при изменениях в файловой системе будет загружаться
    //    повторно
    //  После загрузки ресурсом владеет ownerQueue.
    //  Буфер будет создан как DataBuffer::STORAGE_BUFFER
    //  Потокобезопасный метод.
    ConstRef<TechniqueResource> loadImmediately(
                                          const std::filesystem::path& filePath,
                                          CommandQueueTransfer& ownerQueue);

    //  Асинхронная загрузка данных через отдельный поток.
    //  Если данные уже были загружены ранее, то возвращает их немедленно,
    //    иначе запустит асинхронную задачу на загрузку текстуры и вернет
    //    пустой ресурс.
    //  После загрузки ресурсом владеет ownerQueue.
    //  Буфер будет создан как DataBuffer::STORAGE_BUFFER
    //  Потокобезопасный метод.
    ConstRef<TechniqueResource> sheduleLoading(
                                      const std::filesystem::path& filePath,
                                      CommandQueueTransfer& ownerQueue);

    //  Удалить буферы, на которые нет внешних ссылок.
    //  Потокобезопасный метод.
    void removeUnused() noexcept;

  private:
    // Информация о буфере, который хранитсф в менеджере
    struct ResourceRecord
    {
      //  Ресурс, который возвращается через sheduleLoading
      Ref<TechniqueResource> waitableResource;
      //  Ресурс, который возвращается через loadImmediately
      Ref<TechniqueResource> immediatellyResource;

      //  Асинхронная таска на загрузку с диска
      std::unique_ptr<AsyncTaskQueue::TaskHandle> loadingHandle;

      ResourceRecord() noexcept = default;
      ResourceRecord(const ResourceRecord&) = delete;
      ResourceRecord(ResourceRecord&&) noexcept = default;
      ResourceRecord& operator = (const ResourceRecord&) = delete;
      ResourceRecord& operator = (ResourceRecord&&) noexcept = default;
      ~ResourceRecord() noexcept = default;
    };

    //  Ключ для мапы ресурсов
    struct ResourcesKey
    {
      std::filesystem::path filePath;
      CommandQueueTransfer* commandQueue;

      std::strong_ordering operator <=> (
                                  const ResourcesKey&) const noexcept = default;
    };
    struct KeyHash
    {
      std::size_t operator()(const ResourcesKey& key) const
      {
        return std::hash<std::filesystem::path>()(key.filePath) ^
                std::hash<CommandQueueTransfer*>()(key.commandQueue);
      }
    };
    using ResourcesMap = std::unordered_map<ResourcesKey,
                                            ResourceRecord,
                                            KeyHash>;

    //  Асинхронная таска для загрузки текстуры в отдельном потоке
    class LoadingTask;

  private:
    //  Обработа событий от FileWatcher
    virtual void onFileChanged( const std::filesystem::path& filePath,
                                EventType eventType) override;

    //  Найти подходящий ресурс для loadImmediately
    ResourceRecord* _getExistingResource( const std::filesystem::path& filePath,
                                          CommandQueueTransfer& ownerQueue);

    //  Создать новый ресурс для для loadImmediately
    ResourceRecord& _createNewRecord( const std::filesystem::path& filePath,
                                      CommandQueueTransfer& ownerQueue,
                                      const DataBuffer* image);

    void _addFileWatching(const std::filesystem::path& filePath) noexcept;

    //  Обновить буфер для всех подходящийх ресурсов.
    //  Вызывается из LoadingTask
    void _updateResource( const std::filesystem::path& filePath,
                          CommandQueueTransfer& ownerQueue,
                          const DataBuffer& buffer);

    //  Посчитать, сколько ресурсов ссылается на файл
    int _getRecordsCount(const std::filesystem::path& filePath) const noexcept;

  private:
    FileWatcher& _fileWatcher;
    AsyncTaskQueue& _loadingQueue;

    ResourcesMap _resources;

    mutable std::mutex _accessMutex;
  };
}