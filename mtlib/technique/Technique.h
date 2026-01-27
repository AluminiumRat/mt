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
  class CommandProducerGraphic;
  class CommandProducerTransfer;
  class CommandQueueTransfer;
  class Device;

  class Technique
  {
  public:
    //  RAII обертка вокруг биндинга техники к комманд продюсеру
    //  Автоматически анбиндит технику в деструкторе
    class Bind
    {
    public:
      inline Bind() noexcept;
      inline Bind(const Technique& technique,
                  const TechniquePass& pass,
                  CommandProducerGraphic& producer,
                  const TechniqueVolatileContext* volatileContext = nullptr);
      Bind(const Bind&) = delete;
      inline Bind(Bind&& other) noexcept;
      Bind& operator = (const Bind&) = delete;
      inline Bind& operator == (Bind&& other) noexcept;
      inline ~Bind() noexcept;

      inline void release() noexcept;
      inline bool isValid() const noexcept;

    private:
      const Technique* _technique;
      CommandProducerGraphic* _graphicProducer;
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

    //  Подключить пайплайн и все необходимые ресурсы к продюсеро
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько техник к одному
    //    продюсеру.
    bool bindGraphic(
              CommandProducerGraphic& producer,
              const TechniquePass& pass,
              const TechniqueVolatileContext* volatileContext = nullptr) const;
    void unbindGraphic(CommandProducerGraphic& producer) const noexcept;

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
    void _bindDescriptorsGraphic(
                        CommandProducerGraphic& producer,
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

  inline Technique::Bind::Bind() noexcept :
    _technique(nullptr),
    _graphicProducer(nullptr)
  {
  }

  inline Technique::Bind::Bind(
                            const Technique& technique,
                            const TechniquePass& pass,
                            CommandProducerGraphic& producer,
                            const TechniqueVolatileContext* volatileContext) :
    _technique(nullptr),
    _graphicProducer(nullptr)
  {
    if(technique.bindGraphic(producer, pass, volatileContext))
    {
      _technique = &technique;
      _graphicProducer = &producer;
    }
  }

  inline Technique::Bind::Bind(Bind&& other) noexcept :
    _technique(other._technique),
    _graphicProducer(other._graphicProducer)
  {
    other._technique = nullptr;
    other._graphicProducer = nullptr;
  }

  inline Technique::Bind& Technique::Bind::operator == (Bind&& other) noexcept
  {
    release();
    _technique = other._technique;
    other._technique = nullptr;
    _graphicProducer = other._graphicProducer;
    other._graphicProducer = nullptr;
  }

  inline Technique::Bind::~Bind() noexcept
  {
    release();
  }

  inline void Technique::Bind::release() noexcept
  {
    if(_technique != nullptr)
    {
      _technique->unbindGraphic(*_graphicProducer);
      _technique = nullptr;
      _graphicProducer = nullptr;
    }
  }

  inline bool Technique::Bind::isValid() const noexcept
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