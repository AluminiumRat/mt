#pragma once

#include <array>
#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <util/RefCounter.h>
#include <util/Ref.h>
#include <util/SpinLock.h>
#include <technique/DescriptorSetType.h>
#include <technique/PassConfigurator.h>
#include <technique/ShaderCompilator.h>
#include <technique/TechniqueConfiguration.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/FrameBufferFormat.h>

namespace mt
{
  struct ConfigurationBuildContext;
  class Device;
  class Technique;

  //  Класс для настройки конфигурации техник (TechniqueConfiguration).
  //  Основной паттерн применения:
  //    Пользователь создает конфигуратор и настраивает его. После этого
  //      создает новую конфигурацию, вызвав rebuildConfiguation
  //    Техника подключается к конфигуратору и получает от него конфигурацию.
  //    Далее техника использует пайплайны из конфигурации для отрисовки или
  //      вычислений
  //    Техика при работе отслеживает ревизию конфигуратора, и если произошли
  //      изменения, то получает от конфигуратора новую конфигурацию
  //    Пользователь может изменить конфигурацию, перенастроив конфигуратор и
  //      вызвав rebuildConfiguration
  class TechniqueConfigurator : public RefCounter
  {
  public:
    //  Настройки для отдельных проходов техники
    using Passes = std::vector<std::unique_ptr<PassConfigurator>>;

    //  Дефайн в шейдере, который может принимать только ограниченное количество
    //  значений. Позволяет заранее скомпилировать все возможные вариации
    //  пайплайна и во время рендера просто выбирать нужный.
    struct SelectionDefine
    {
      std::string name;
      std::vector<std::string> valueVariants;
    };
    using Selections = std::vector<SelectionDefine>;

    using DefaultSampler = TechniqueConfiguration::DefaultSampler;
    using Samplers = TechniqueConfiguration::Samplers;

  public:
    TechniqueConfigurator(Device& device, const char* debugName);
    TechniqueConfigurator(const TechniqueConfigurator&) = delete;
    TechniqueConfigurator& operator = (const TechniqueConfigurator&) = delete;
  protected:
    ~TechniqueConfigurator() noexcept = default;
  public:

    inline Device& device() const noexcept;

    //  Может возвращать nullptr, если сборка конфигурации не завершилась
    //    успешно (например, ошибка в шейдере или неполный набор шейдеров) или
    //    конфигурация ещё не была собрана
    //  Доступ к текущей конфигурации потокобезопасен. После того как
    //    конфигурация собрана и выставлена в конфигуратор как текущая, она не
    //    изменяется.
    inline ConstRef<TechniqueConfiguration> configuration() const noexcept;

    //  Пересобрать и раздать конфигурацию всем зависимым техникам.
    //  usedFiles - контейнер, в который будут добавляться все использованные
    //    при компиляции файлы, включая file. Можно передать nullptr.
    //  Если скомпилировать шейдер не удалось, и метод выбрасывает исключение,
    //    то в контейнер usedFiles всё равно будут добавлены те файлы, которые
    //    удалось распознать.
    //  ВНИМАНИЕ! Метод не чистит usedFiles перед работой, только добавляет
    //    новые файлы.
    //  ВНИМАНИЕ!!! Настройка конфигуратора и сборка конфигурации должны быть
    //    синхронизированны внешне.
    //  ВНИМАНИЕ!!! Хотя доступ к конфигурации и является потокобезопасным,
    //    но обновление техник не потокобезопасно. Распространение конфигурации
    //    должно быть внешне синхронизированно с использованием техник.
    void rebuildConfiguration(
                std::unordered_set<std::filesystem::path>* usedFiles = nullptr);

    //  Пересобрать конфигурацию без раздачи техникам. Метод может быть вызван
    //    в любое время из любого потока, но настройка конфигуратора и
    //    и сборка конфигурации должны быть синхронизированны внешне.
    //  usedFiles - контейнер, в который будут добавляться все использованные
    //    при компиляции файлы, включая file. Можно передать nullptr.
    //  Если скомпилировать шейдер не удалось, и метод выбрасывает исключение,
    //    то в контейнер usedFiles всё равно будут добавлены те файлы, которые
    //    удалось распознать.
    //  ВНИМАНИЕ! Метод не чистит usedFiles перед работой, только добавляет
    //    новые файлы.
    void rebuildOnlyConfiguration(
                std::unordered_set<std::filesystem::path>* usedFiles = nullptr);

    //  Сообщить зависимым техникам, что конфигурация поменялась.
    //  ВНИМАНИЕ!!! Хотя доступ к конфигурации и является потокобезопасным,
    //    но обновление техник не потокобезопасно. Распространение конфигурации
    //    должно быть внешне синхронизированно с использованием техник.
    void propogateConfiguration() noexcept;

    inline const std::string& debugName() const noexcept;

    inline const Passes& passes() const noexcept;
    inline void setPasses(Passes&& newPasses) noexcept;
    inline void addPass(std::unique_ptr<PassConfigurator> newPass);

    inline const Selections& selections() const noexcept;
    inline void setSelections(std::span<const SelectionDefine> newSelections);

    inline const Samplers& defaultSamplers() const noexcept;
    inline void setDefaultSamplers(std::span<const DefaultSampler> newSamplers);

    //  Удалить все проходы, селекшены и дефолтные сэмплеры
    inline void clear() noexcept;

  private:
    //  Работа с обсерверами потокобезопасна. Зависимые техники можно создавать
    //  в любом потоке когда угодно
    friend class Technique;
    inline void addObserver(Technique& technique);
    inline void removeObserver(Technique& technique) noexcept;

  private:
    void _createSelections(ConfigurationBuildContext& context) const;
    //  Создать и обработать проходы в конфигурации (без создания пайплайнов)
    void _createPasses(ConfigurationBuildContext& context) const;
    //  По пропарсенным данным из рефлексии создаем лэйауты для
    //  дескриптер сетов и пайплайнов
    void _createLayouts(ConfigurationBuildContext& context) const;
    //  Финальная стадия построения конфигурации - создаем все варианты
    //  пайплайнов
    void _createPipelines(ConfigurationBuildContext& context) const;

  private:
    Device& _device;

    std::string _debugName;

    Ref<TechniqueConfiguration> _configuration;
    mutable SpinLock _configurationMutex;

    Passes _passes;

    Selections _selections;

    Samplers _defaultSamplers;

    using Observers = std::vector<Technique*>;
    Observers _observers;
    mutable SpinLock _observersMutex;
  };

  inline Device& TechniqueConfigurator::device() const noexcept
  {
    return _device;
  }

  inline ConstRef<TechniqueConfiguration>
                          TechniqueConfigurator::configuration() const noexcept
  {
    std::lock_guard lock(_configurationMutex);
    return _configuration;
  }

  inline const std::string& TechniqueConfigurator::debugName() const noexcept
  {
    return _debugName;
  }

  inline void TechniqueConfigurator::addObserver(Technique& technique)
  {
    std::lock_guard lock(_observersMutex);
    _observers.push_back(&technique);
  }

  inline void TechniqueConfigurator::removeObserver(
                                                  Technique& technique) noexcept
  {
    std::lock_guard lock(_observersMutex);
    for(Observers::iterator iObserver = _observers.begin();
        iObserver != _observers.end();
        iObserver++)
    {
      if(*iObserver == &technique)
      {
        _observers.erase(iObserver);
        return;
      }
    }
  }

  inline const TechniqueConfigurator::Passes&
                                TechniqueConfigurator::passes() const noexcept
  {
    return _passes;
  }

  inline void TechniqueConfigurator::setPasses(Passes&& newPasses) noexcept
  {
    _passes = std::move(newPasses);
  }

  inline void TechniqueConfigurator::addPass(
                                    std::unique_ptr<PassConfigurator> newPass)
  {
    _passes.push_back(std::move(newPass));
  }


  inline const std::vector<TechniqueConfigurator::SelectionDefine>&
                            TechniqueConfigurator::selections() const noexcept
  {
    return _selections;
  }

  inline void TechniqueConfigurator::setSelections(
                                std::span<const SelectionDefine> newSelections)
  {
    Selections newSelectionsVector( newSelections.begin(), newSelections.end());
    _selections = std::move(newSelectionsVector);
  }

  inline const TechniqueConfigurator::Samplers&
                        TechniqueConfigurator::defaultSamplers() const noexcept
  {
    return _defaultSamplers;
  }

  inline void TechniqueConfigurator::setDefaultSamplers(
                                    std::span<const DefaultSampler> newSamplers)
  {
    Samplers newSamplersVector(newSamplers.begin(), newSamplers.end());
    _defaultSamplers = std::move(newSamplersVector);
  }

  inline void TechniqueConfigurator::clear() noexcept
  {
    _passes.clear();
    _selections.clear();
    _defaultSamplers.clear();
  }
}