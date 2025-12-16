#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/Technique.h>
#include <technique/TechniqueConfigurator.h>
#include <util/Assert.h>
#include <util/Ref.h>

namespace mt
{
  class Device;

  //  Класс для централизованной загрузки и обновления техник (класс Technique)
  //  Сама техника не является разделяемым ресурсом, для каждого вызова
  //    loadImmediately или scheduleLoading создается отдельный объект
  //    Technique. Вы можете использовать как константные так и неконстантные
  //    методы этого класса.
  //  Разделяемым ресурсом является TechniqueConfigurator
  //  Все методы класса потокобезопасные. Однако учтите, что обновление
  //    техник (то есть передача им новых конфигураций) производится в
  //    синхронной части AsyncTaskQueue
  class TechniqueManager
  {
  public:
    TechniqueManager(FileWatcher& fileWatcher, AsyncTaskQueue& loadingQueue);
    TechniqueManager(const TechniqueManager&) = delete;
    TechniqueManager& operator = (const TechniqueManager&) = delete;
    ~TechniqueManager() noexcept = default;

    //  Попытаться загрузить технику немедленно.
    //  Не может вернуть nullptr
    //  Может вернуть технику без собранных пайплайнов (но привязанную к
    //    конфигуратору), если не удается загрузить технику (нет файла,
    //    например).
    //  Независимо от того, удастся ли загрузить технику, соответствующие файлы
    //    будут направлены на отслеживание и при изменениях в файловой системе
    //    техника будет загружаться повторно
    Ref<Technique> loadImmediately( const std::filesystem::path& filePath,
                                    Device& device);

    //  Асинхронная загрузка техники через отдельный поток.
    //  Никогда не возвращает nullptr
    //  Если конфигуратор уже был загружен ранее, то возвращает технику
    //    немедленно, иначе запустит асинхронную задачу на загрузку текстуры
    //    и вернет технику без собранных пайплайнов.
    Ref<Technique> scheduleLoading( const std::filesystem::path& filePath,
                                    Device& device);

    //  Удалить конфигураторы, на которые нет внешних ссылок.
    void removeUnused() noexcept;

  private:
    class TechniqueResource : private FileObserver
    {
    public:
      TechniqueResource(const std::filesystem::path& filePath,
                        Device& device,
                        TechniqueManager& manager);
      TechniqueResource(const TechniqueResource&) = delete;
      TechniqueResource& operator = (const TechniqueResource&) = delete;
      virtual ~TechniqueResource() noexcept;

      //  Только загрузка из файла и сборка конфигурации. Без раздачи техникам.
      void rebuildConfiguration();
      void propagateConfiguration();
      //  Создать асинхронную таску для пересборки конфигурации
      void scheduleRebuild();
      //  Есть ли внешние (кроме менеджера) ссылки на ресурс
      bool used() const noexcept;
      //  Создать технику из конфигуратора.
      //  checkProcessed - если true, то проверяем флаг _processed и загружаем
      //    конфигуратор при необходимости
      Ref<Technique> createTechnique(bool checkProcessed);

    private:
      virtual void onFileChanged( const std::filesystem::path& filePath,
                                  EventType eventType) override;
      void _rebuild();

    private:
      TechniqueManager& _manager;

      std::filesystem::path _filePath;

      Ref<TechniqueConfigurator> _configurator;
      //  Флаг, говорящий о том, что уже была попытка загрузить конфигурацию
      bool _processed;
      //  Защищает _configurator и _processed. _loadingHandle защищается через
      //  TechniqueManager::_accessMutex
      mutable std::mutex _configuratorMutex;

      std::unique_ptr<AsyncTaskQueue::TaskHandle> _loadingHandle;
    };

    //  Ключ для мапы ресурсов
    struct ResourcesKey
    {
      std::filesystem::path filePath;
      Device* device;

      std::strong_ordering operator <=> (
                                  const ResourcesKey&) const noexcept = default;
    };
    struct KeyHash
    {
      std::size_t operator()(const ResourcesKey& key) const
      {
        return std::hash<std::filesystem::path>()(key.filePath) ^
                                              std::hash<Device*>()(key.device);
      }
    };
    using ResourcesMap = std::unordered_map<ResourcesKey,
                                            std::unique_ptr<TechniqueResource>,
                                            KeyHash>;
    class RebuildTask;

  private:
    FileWatcher& _fileWatcher;
    AsyncTaskQueue& _loadingQueue;

    ResourcesMap _resources;

    mutable std::mutex _accessMutex;
  };
}