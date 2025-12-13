#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/TechniqueResource.h>
#include <vkr/image/ImageView.h>
#include <util/Ref.h>

namespace mt
{
  class CommandQueueTransfer;
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
    ~TextureManager() noexcept;

    //  Попытаться загрузить текстуру немедленно
    //  Даже если текстура не будет загружена(нет файла, либо неверный/
    //    неподдерживаемый формат), файл будет направлен на отслеживание и
    //    при изменениях в файловой системе будет загружаться повторно
    //  Потокобезопасный метод.
    //  useDefaultTexture - определяет поведение на случай, если файл загрузить
    //    не удалось. Если true, то будет возвращена непустая дефолтная
    //    текстура. Если false, то будет возвращен пустой ресурс.
    //  Загруженный Image не поддерживает автоконтроль лэйаутов, его лэйаут
    //    выставлен в VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ресурсом владеет
    //    ownerQueue. Image гарантированно поддерживает сэмплирование в шейдерах
    //    и копирование данных из него.
    ConstRef<TechniqueResource> loadImmediately(
                                      const std::filesystem::path& filePath,
                                      CommandQueueTransfer& ownerQueue,
                                      bool useDefaultTexture);

    //  Асинхронная загрузка текстуры через отдельный поток.
    //  Если текстура уже была загружена ранее, то возвращает её немедленно,
    //    иначе запустит асинхронную задачу на загрузку текстуры и вернет
    //    либо ресурс с дефолтной image, либо ресурс с nullptr image  в
    //    зависимости от параметра useDefaultTexture
    //  Потокобезопасный метод.
    //  useDefaultTexture определяет, что будет находится в ресурсе до первой
    //    удачной загрузки. Если true, то будет использована непустая дефолтная
    //    текстура. Если false, то будет возвращен пустой ресурс.
    //  Загруженный Image не поддерживает автоконтроль лэйаутов, его лэйаут
    //    выставлен в VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ресурсом владеет
    //    ownerQueue. Image гарантированно поддерживает сэмплирование в шейдерах
    //    и копирование данных из него.
    ConstRef<TechniqueResource> sheduleLoading(
                                      const std::filesystem::path& filePath,
                                      CommandQueueTransfer& ownerQueue,
                                      bool useDefaultTexture);

    //  Удалить все текстуры, на которые нет внешних ссылок.
    //  Потокобезопасный метод.
    void removeUnused() noexcept;

  private:
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

    struct ResourceRecord
    {
      //  Оба ресурса используют общий imageView после того как текстура удачно
      //    загружена с диска, здесь нет дублирования.
      //  Ресурс, для которого попросили использовать дефолтную текстуру
      Ref<TechniqueResource> withDefault;
      //  Ресурс без использования дефолтной текстуры
      Ref<TechniqueResource> noDefault;

      //  Асинхронная таска на загрузку текстуры с диска
      std::unique_ptr<AsyncTaskQueue::TaskHandle> loadingHandle;

      ResourceRecord() noexcept = default;
      ResourceRecord(const ResourceRecord&) = delete;
      ResourceRecord(ResourceRecord&&) noexcept = default;
      ResourceRecord& operator = (const ResourceRecord&) = delete;
      ResourceRecord& operator = (ResourceRecord&&) noexcept = default;
      ~ResourceRecord() noexcept = default;
    };

    using ResourcesMap = std::unordered_map<ResourcesKey,
                                            ResourceRecord,
                                            KeyHash>;

    //  Асинхронная таска для загрузки текстуры в отдельном потоке
    class LoadTextureTask;

  private:
    TechniqueResource* _getExistingResource(
                                          const std::filesystem::path& filePath,
                                          CommandQueueTransfer& ownerQueue,
                                          bool useDefaultTexture) const;

    //  Обработа событий от FileWatcher
    virtual void onFileChanged( const std::filesystem::path& filePatch,
                                EventType eventType) override;

    //  Вызывается из LoadTextureTask
    void _updateTexture(const std::filesystem::path& filePath,
                        CommandQueueTransfer& ownerQueue,
                        const ImageView& imageView);

    //  Посчитать, сколько ресурсов ссылается на файл
    int _getRecordsCount(const std::filesystem::path& filePath) const noexcept;

  private:
    FileWatcher& _fileWatcher;
    AsyncTaskQueue& _loadingQueue;

    ResourcesMap _resourcesMap;

    mutable std::mutex _accessMutex;
  };
}