#pragma once

#include <memory>
#include <string>
#include <vector>

#include <technique/ResourceBinding.h>
#include <technique/Selection.h>
#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/TechniquePass.h>
#include <technique/TechniqueUniformBlock.h>
#include <technique/TechniqueVolatileContext.h>
#include <technique/UniformVariable.h>
#include <util/Ref.h>
#include <util/Signal.h>
#include <util/SpinLock.h>
#include <vkr/pipeline/DescriptorPool.h>
#include <vkr/pipeline/DescriptorSet.h>

namespace mt
{
  class CommandProducer;
  class CommandProducerCompute;
  class CommandProducerGraphic;
  class CommandProducerTransfer;
  class CommandQueueTransfer;
  class Device;

  class Technique
  {
  public:
    //  RAII обертка вокруг биндинга графической техники к комманд продюсеру
    //  Автоматически анбиндит технику в деструкторе
    class BindGraphic
    {
    public:
      inline BindGraphic() noexcept;
      inline BindGraphic(
                    const Technique& technique,
                    const TechniquePass& pass,
                    CommandProducerGraphic& producer,
                    const TechniqueVolatileContext* volatileContext = nullptr);
      BindGraphic(const BindGraphic&) = delete;
      inline BindGraphic(BindGraphic&& other) noexcept;
      BindGraphic& operator = (const BindGraphic&) = delete;
      inline BindGraphic& operator == (BindGraphic&& other) noexcept;
      inline ~BindGraphic() noexcept;

      inline void release() noexcept;
      inline bool isValid() const noexcept;

    private:
      const Technique* _technique;
      CommandProducerGraphic* _producer;
    };

    //  RAII обертка вокруг биндинга компьют техники к комманд продюсеру
    //  Автоматически анбиндит технику в деструкторе
    class BindCompute
    {
    public:
      inline BindCompute() noexcept;
      inline BindCompute(
                    const Technique& technique,
                    const TechniquePass& pass,
                    CommandProducerCompute& producer,
                    const TechniqueVolatileContext* volatileContext = nullptr);
      BindCompute(const BindCompute&) = delete;
      inline BindCompute(BindCompute&& other) noexcept;
      BindCompute& operator = (const BindCompute&) = delete;
      inline BindCompute& operator == (BindCompute&& other) noexcept;
      inline ~BindCompute() noexcept;

      inline void release() noexcept;
      inline bool isValid() const noexcept;

    private:
      const Technique* _technique;
      CommandProducerCompute* _producer;
    };

  public:
    //  Создать технику и привязать её к конфигуратору.
    //  Техника будет автоматически обновляться каждый раз, когда конфигуратор
    //    будет успешно собирать новую конфигурацию.
    //  Дебажное имя берется из конфигуратора
    explicit Technique(TechniqueConfigurator& configurator);
    //  Создать технику, не привязываясь к конфигуратору. Позволяет избежать
    //    автообновлений и использовать технику в асинхронных тасках.
    //  ВНИМАНИЕ! Для того чтобы использовать технику в асинхронных задачах
    //    также необходимо проследить, что к ней не присоединен ни один ресурс
    //    (ResourceBinding::setResource), так как ресурсы так же автоматически
    //    обнавляют состояние техники
    Technique(const TechniqueConfiguration& configuration,
              const char* debugName);
    Technique(const Technique&) = delete;
    Technique& operator = (const Technique&) = delete;
    virtual ~Technique() noexcept;

    inline Device& device() const noexcept;

    //  Волатильный контекст позволяет донастроить технику перед отрисовкой
    //    через константные методы классов Slection, ResourceBinding и
    //    UniformVariable
    TechniqueVolatileContext createVolatileContext(
                                              CommandProducer& producer) const;

    //  Проверка, готова ли техника к использованию.
    //  Проверяем, что конфигурация была успешно собрана и подключена к
    //    технике, а так же к технике подключены все необходимые ресурсы.
    //  Ресурсы из COMMON сета не проверяются на наличие
    bool isReady(
              const TechniqueVolatileContext* volatileContext = nullptr) const;

    //  Подключить графический пайплайн и все необходимые ресурсы к продюсеру
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько графических техник
    //    к одному продюсеру.
    bool bindGraphic(
              CommandProducerGraphic& producer,
              const TechniquePass& pass,
              const TechniqueVolatileContext* volatileContext = nullptr) const;
    void unbindGraphic(CommandProducerGraphic& producer) const noexcept;

    //  Подключить компьют пайплайн и все необходимые ресурсы к продюсеру
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько компьют техник
    //    к одному продюсеру.
    bool bindCompute(
              CommandProducerCompute& producer,
              const TechniquePass& pass,
              const TechniqueVolatileContext* volatileContext = nullptr) const;
    void unbindCompute(CommandProducerCompute& producer) const noexcept;

    Selection& getOrCreateSelection(const char* selectionName);
    Selection* getSelection(const char* selectionName) noexcept;
    const Selection* getSelection(const char* selectionName) const noexcept;

    ResourceBinding& getOrCreateResourceBinding(const char* resourceName);
    ResourceBinding* getResourceBinding(const char* resourceName) noexcept;
    const ResourceBinding* getResourceBinding(
                                      const char* resourceName) const noexcept;

    UniformVariable& getOrCreateUniform(const char* uniformFullName);
    UniformVariable* getUniform(const char* uniformFullName) noexcept;
    const UniformVariable* getUniform(
                                    const char* uniformFullName) const noexcept;

    TechniquePass& getOrCreatePass(const char* passName);
    TechniquePass* getPass(const char* passName) noexcept;
    const TechniquePass* getPass(const char* passName) const noexcept;

    inline const std::string& debugName() const noexcept;

    inline const TechniqueConfigurator& configurator() const noexcept;
    inline const TechniqueConfiguration* configuration() const noexcept;

  public:
    Signal<> configurationUpdatedSignal;

  private:
    friend class TechniqueConfigurator;
    void updateConfiguration();

  private:
    void _setConfiguration(const TechniqueConfiguration& configuration);
    void _updateStaticSet(CommandProducerTransfer& commandProducer) const;
    bool _checkSetReady(DescriptorSetType setType) const noexcept;
    void _bindResources(DescriptorSet& descriptorSet,
                        DescriptorSetType setType,
                        CommandProducerTransfer& commandProducer) const;
    template<typename CommandProducerType>
    void _bindDescriptors(
                        CommandProducerType& producer,
                        const TechniqueVolatileContext* volatileContext) const;
    //  Определить по селекшенам, какой именно вариант пайплайны мы должны
    //  использовать
    uint32_t _getPipelineVariant(
                const TechniqueVolatileContext* volatileContext,
                uint32_t passIndex) const noexcept;

  private:
    Device& _device;

    std::string _debugName;

    Ref<TechniqueConfigurator> _configurator;
    ConstRef<TechniqueConfiguration> _configuration;
    const TechniqueConfiguration::DescriptorSet* _volatileSetDescription;
    const TechniqueConfiguration::DescriptorSet* _staticSetDescription;

    std::vector<std::unique_ptr<SelectionImpl>> _selections;
    size_t _lastProcessedSelectionsRevision;

    std::vector<std::unique_ptr<ResourceBindingImpl>> _resources;
    size_t _resourcesRevision;

    using UniformBlocks = std::vector<std::unique_ptr<TechniqueUniformBlock>>;
    UniformBlocks _uniformBlocks;

    std::vector<std::unique_ptr<UniformVariableImpl>> _uniforms;

    std::vector<std::unique_ptr<TechniquePassImpl>> _passes;

    //  Статик сет использует ленивую инициализацию, но сам используется в
    //  константных методах. Для защиты в многопоточном рендере используем
    //  мьютекс на спинлоке
    mutable Ref<DescriptorPool> _staticPool;
    mutable Ref<DescriptorSet> _staticSet;
    //  В статик сете присутствуют все необходимые ресурсы
    mutable bool _staticSetComplete;
    mutable size_t _lastProcessedResourcesRevision;
    mutable SpinLock _staticSetMutex;
  };

  inline Technique::BindGraphic::BindGraphic() noexcept :
    _technique(nullptr),
    _producer(nullptr)
  {
  }

  inline Technique::BindGraphic::BindGraphic(
                            const Technique& technique,
                            const TechniquePass& pass,
                            CommandProducerGraphic& producer,
                            const TechniqueVolatileContext* volatileContext) :
    _technique(nullptr),
    _producer(nullptr)
  {
    if(technique.bindGraphic(producer, pass, volatileContext))
    {
      _technique = &technique;
      _producer = &producer;
    }
  }

  inline Technique::BindGraphic::BindGraphic(BindGraphic&& other) noexcept :
    _technique(other._technique),
    _producer(other._producer)
  {
    other._technique = nullptr;
    other._producer = nullptr;
  }

  inline Technique::BindGraphic& Technique::BindGraphic::operator == (
                                                  BindGraphic&& other) noexcept
  {
    release();
    _technique = other._technique;
    other._technique = nullptr;
    _producer = other._producer;
    other._producer = nullptr;
  }

  inline Technique::BindGraphic::~BindGraphic() noexcept
  {
    release();
  }

  inline void Technique::BindGraphic::release() noexcept
  {
    if(_technique != nullptr)
    {
      _technique->unbindGraphic(*_producer);
      _technique = nullptr;
      _producer = nullptr;
    }
  }

  inline bool Technique::BindGraphic::isValid() const noexcept
  {
    return _technique != nullptr;
  }

  inline Technique::BindCompute::BindCompute() noexcept :
    _technique(nullptr),
    _producer(nullptr)
  {
  }

  inline Technique::BindCompute::BindCompute(
                            const Technique& technique,
                            const TechniquePass& pass,
                            CommandProducerCompute& producer,
                            const TechniqueVolatileContext* volatileContext) :
    _technique(nullptr),
    _producer(nullptr)
  {
    if (technique.bindCompute(producer, pass, volatileContext))
    {
      _technique = &technique;
      _producer = &producer;
    }
  }

  inline Technique::BindCompute::BindCompute(BindCompute&& other) noexcept :
    _technique(other._technique),
    _producer(other._producer)
  {
    other._technique = nullptr;
    other._producer = nullptr;
  }

  inline Technique::BindCompute& Technique::BindCompute::operator == (
    BindCompute&& other) noexcept
  {
    release();
    _technique = other._technique;
    other._technique = nullptr;
    _producer = other._producer;
    other._producer = nullptr;
  }

  inline Technique::BindCompute::~BindCompute() noexcept
  {
    release();
  }

  inline void Technique::BindCompute::release() noexcept
  {
    if (_technique != nullptr)
    {
      _technique->unbindCompute(*_producer);
      _technique = nullptr;
      _producer = nullptr;
    }
  }

  inline bool Technique::BindCompute::isValid() const noexcept
  {
    return _technique != nullptr;
  }

  inline Device& Technique::device() const noexcept
  {
    return _device;
  }

  inline const std::string& Technique::debugName() const noexcept
  {
    return _debugName;
  }

  inline const TechniqueConfigurator& Technique::configurator() const noexcept
  {
    return *_configurator;
  }

  inline const TechniqueConfiguration* Technique::configuration() const noexcept
  {
    return _configuration.get();
  }
}