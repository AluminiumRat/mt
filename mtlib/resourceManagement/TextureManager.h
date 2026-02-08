#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/TechniqueResource.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandQueueGraphic;
  class Device;

  //  Класс используется для централизованной загрузки текстур с диска и
  //    слежением за изменениями текстур на диске. Также позволяет расшаривать
  //    текстуры между многими потребителями, избега дублирования данных.
  //  Все методы класса потокобезопасные. Однако учтите, что обновление
  //    ресурсов (то есть передача им новых ImageView) производится в синхронной
  //    части AsyncTaskQueue
  class TextureManager : private FileObserver
  {
  public:
    //  loadingQueue - очередь, которая будет использоваться для загрузки
    //    текстур с диска. Так же в синхронной части этой очереди
    //    (AsyncTaskQueue::update) будет происходить одновление ресурсов, для
    //    которых загружены новые Image
    TextureManager( FileWatcher& fileWatcher, AsyncTaskQueue& loadingQueue);
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator = (const TextureManager&) = delete;
    virtual ~TextureManager() noexcept;

    //  Попытаться загрузить текстуру немедленно.
    //  Никогда не возвращает nullptr
    //  Может вернуть пустой ресурс или ресурс с дефолтной текстурой,
    //    если не удается загрузить текстуру (нет файла, либо
    //    неверный/неподдерживаемый формат).
    //  Независимо от того, будет ли загружена текстура, файл будет направлен
    //    на отслеживание и при изменениях в файловой системе будет загружаться
    //    повторно
    //  useDefaultTexture - определяет поведение на случай, если файл загрузить
    //    не удалось. Если true, то будет возвращена непустая дефолтная
    //    текстура. Если false, то будет возвращен пустой ресурс.
    //  Загруженный Image не поддерживает автоконтроль лэйаутов, его лэйаут
    //    выставлен в VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ресурсом владеет
    //    uploadingQueue. Image гарантированно поддерживает сэмплирование в
    //    шейдерах и копирование данных из него.
    //  Потокобезопасный метод.
    ConstRef<TechniqueResource> loadImmediately(
                                          const std::filesystem::path& filePath,
                                          CommandQueueGraphic& uploadingQueue,
                                          bool useDefaultTexture);

    //  Асинхронная загрузка текстуры через отдельный поток.
    //  Никогда не возвращает nullptr
    //  Если текстура уже была загружена ранее, то возвращает её немедленно,
    //    иначе запустит асинхронную задачу на загрузку текстуры и вернет
    //    либо ресурс с дефолтной image, либо ресурс с nullptr image  в
    //    зависимости от параметра useDefaultTexture
    //  useDefaultTexture определяет, что будет находится в ресурсе до первой
    //    удачной загрузки. Если true, то будет использована непустая дефолтная
    //    текстура. Если false, то будет возвращен пустой ресурс.
    //  Загруженный Image не поддерживает автоконтроль лэйаутов, его лэйаут
    //    выставлен в VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ресурсом владеет
    //    uploadingQueue. Image гарантированно поддерживает сэмплирование в
    //    шейдерах и копирование данных из него.
    //  Потокобезопасный метод.
    ConstRef<TechniqueResource> scheduleLoading(
                                      const std::filesystem::path& filePath,
                                      CommandQueueGraphic& uploadingQueue,
                                      bool useDefaultTexture);

    //  Удалить все текстуры, на которые нет внешних ссылок.
    //  Потокобезопасный метод.
    void removeUnused() noexcept;

  private:
    // Информация о текстуре, хранимой в менеджере
    struct ResourceRecord
    {
      //  Ресурсы, которые возвращаются через scheduleLoading
      Ref<TechniqueResource> waitableWithDefault; // С дефолтной текстурой
      Ref<TechniqueResource> waitableNoDefault;   // Без дефолтной текстуры

      //  Ресурсы, которые возвращаются через loadImmediately
      Ref<TechniqueResource>immediatellyWithDefault;  // С дефолтной текстурой
      Ref<TechniqueResource> immediatellyNoDefault;   // Без дефолтной текстуры

      //  Асинхронная таска на загрузку текстуры с диска
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
      CommandQueueGraphic* commandQueue;

      std::strong_ordering operator <=> (
                                  const ResourcesKey&) const noexcept = default;
    };
    struct KeyHash
    {
      std::size_t operator()(const ResourcesKey& key) const
      {
        return std::hash<std::filesystem::path>()(key.filePath) ^
                std::hash<CommandQueueGraphic*>()(key.commandQueue);
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
                                          CommandQueueGraphic& uploadingQueue);

    //  Создать новый ресурс для для loadImmediately
    ResourceRecord& _createNewRecord( const std::filesystem::path& filePath,
                                      CommandQueueGraphic& uploadingQueue,
                                      const ImageView* image);

    //  Добавить в запись record ресурсы для loadImmediately
    void _createImmediatelyResources(
                                    ResourceRecord& record,
                                    const ImageView* image,
                                    CommandQueueGraphic& uploadingQueue) const;

    void _addFileWatching(const std::filesystem::path& filePath) noexcept;

    //  Обновить текстуру для подходящего ресурса.
    //  Вызывается из LoadTextureTask
    void _updateTexture(const std::filesystem::path& filePath,
                        CommandQueueGraphic& uploadingQueue,
                        const ImageView& imageView);

    //  Посчитать, сколько ресурсов ссылается на файл
    int _getRecordsCount(const std::filesystem::path& filePath) const noexcept;

  private:
    FileWatcher& _fileWatcher;
    AsyncTaskQueue& _loadingQueue;

    ResourcesMap _resources;

    mutable std::mutex _accessMutex;
  };
}