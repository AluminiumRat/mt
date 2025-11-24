#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <span>
#include <string>
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
#include <technique/TechniqueGonfiguration.h>
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
  class TechniqueGonfigurator : public RefCounter
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

  public:
    TechniqueGonfigurator(Device& device,
                          const char* debugName = "TechniqueConfigurator");
    TechniqueGonfigurator(const TechniqueGonfigurator&) = delete;
    TechniqueGonfigurator& operator = (const TechniqueGonfigurator&) = delete;
  protected:
    ~TechniqueGonfigurator() noexcept = default;
  public:

    inline Device& device() const noexcept;

    //  Может возвращать nullptr, если сборка конфигурации не завершилась
    //    успешно (например, ошибка в шейдере или неполный набор шейдеров) или
    //    конфигурация ещё не была собрана
    //  Доступ к текущей конфигурации потокобезопасен. После того как
    //    конфигурация собрана и выставлена в конфигуратор как текущая, она не
    //    изменяется.
    inline ConstRef<TechniqueGonfiguration> configuration() const noexcept;

    //  Пересобрать и раздать конфигурацию всем зависимым техникам.
    //  ВНИМАНИЕ!!! Настройка конфигуратора и сборка конфигурации должны быть
    //    синхронизированны внешне.
    //  ВНИМАНИЕ!!! Хотя доступ к конфигурации и является потокобезопасным,
    //    но обновление техник не потокобезопасно. Распространение конфигурации
    //    должно быть внешне синхронизированно с использованием техник.
    void rebuildConfiguration();
    //  Пересобрать конфигурацию без раздачи техникам. Метод может быть вызван
    //    в любое время из любого потока, но настройка конфигуратора и
    //    сборка конфигурации должны быть синхронизированны внешне.
    void rebuildOnlyConfiguration();
    //  Сообщить зависимым техникам, что конфигурация поменялась.
    //  ВНИМАНИЕ!!! Хотя доступ к конфигурации и является потокобезопасным,
    //    но обновление техник не потокобезопасно. Распространение конфигурации
    //    должно быть внешне синхронизированно с использованием техник.
    void propogateConfiguration() noexcept;

    inline const std::string& debugName() const noexcept;

    inline const Passes& passes() const noexcept;
    inline void setPasses(Passes&& newPasses) noexcept;

    inline const std::vector<SelectionDefine>& selections() const noexcept;
    inline void setSelections(std::span<const SelectionDefine> newSelections);

    //  Удалить все проходы и все селекшены
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

    Ref<TechniqueGonfiguration> _configuration;
    mutable SpinLock _configurationMutex;

    Passes _passes;

    std::vector<SelectionDefine> _selections;

    using Observers = std::vector<Technique*>;
    Observers _observers;
    mutable SpinLock _observersMutex;
  };

  inline Device& TechniqueGonfigurator::device() const noexcept
  {
    return _device;
  }

  inline ConstRef<TechniqueGonfiguration>
                          TechniqueGonfigurator::configuration() const noexcept
  {
    std::lock_guard lock(_configurationMutex);
    return _configuration;
  }

  inline const std::string& TechniqueGonfigurator::debugName() const noexcept
  {
    return _debugName;
  }

  inline void TechniqueGonfigurator::addObserver(Technique& technique)
  {
    std::lock_guard lock(_observersMutex);
    _observers.push_back(&technique);
  }

  inline void TechniqueGonfigurator::removeObserver(
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

  inline const TechniqueGonfigurator::Passes&
                                TechniqueGonfigurator::passes() const noexcept
  {
    return _passes;
  }

  inline void TechniqueGonfigurator::setPasses(Passes&& newPasses) noexcept
  {
    _passes = std::move(newPasses);
  }

  inline const std::vector<TechniqueGonfigurator::SelectionDefine>&
                            TechniqueGonfigurator::selections() const noexcept
  {
    return _selections;
  }

  inline void TechniqueGonfigurator::setSelections(
                                std::span<const SelectionDefine> newSelections)
  {
    std::vector<SelectionDefine> newSelectionsVector( newSelections.begin(),
                                                      newSelections.end());
    _selections = std::move(newSelectionsVector);
  }

  inline void TechniqueGonfigurator::clear() noexcept
  {
    _passes.clear();
    _selections.clear();
  }
}