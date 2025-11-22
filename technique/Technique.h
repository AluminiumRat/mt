#pragma once

#include <memory>
#include <string>
#include <vector>

#include <technique/Selection.h>
#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/TechniqueResource.h>
#include <technique/TechniqueUniformBlock.h>
#include <technique/TechniqueVolatileContext.h>
#include <technique/UniformVariable.h>
#include <util/Ref.h>
#include <util/RefCounter.h>
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

  class Technique : public RefCounter
  {
  public:
    Technique(Device& device,
              const char* debugName = "Technique");
    Technique(const Technique&) = delete;
    Technique& operator = (const Technique&) = delete;
  protected:
    virtual ~Technique() noexcept = default;
  public:

    inline Device& device() const noexcept;

    inline TechniqueConfigurator& configurator() const noexcept;

    //  Волатильный контекст позволяет донастроить технику перед отрисовкой
    //    через константные методы классов Slection, TechniqueResource и
    //    UniformVariable
    TechniqueVolatileContext createVolatileContext(
                                              CommandProducer& producer) const;

    //  Подключить пайплайн и все необходимые ресурсы к продюсеро
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько техник к одному
    //    продюсеру.
    bool bindGraphic(
              CommandProducerGraphic& producer,
              const TechniqueVolatileContext* volatileContext = nullptr) const;
    void unbindGraphic(CommandProducerGraphic& producer) const noexcept;

    Selection& getOrCreateSelection(const char* selectionName);
    TechniqueResource& getOrCreateResource(const char* resourceName);
    UniformVariable& getOrCreateUniform(const char* uniformFullName);

    inline const std::string& debugName() const noexcept;

  private:
    friend class TechniqueConfigurator;
    void updateConfiguration();

  private:
    void _updateStaticSet(CommandProducerTransfer& commandProducer) const;
    void _bindResources(DescriptorSet& descriptorSet,
                        DescriptorSetType setType,
                        CommandProducerTransfer& commandProducer) const;
    void _bindDescriptorsGraphic(
                        CommandProducerGraphic& producer,
                        const TechniqueVolatileContext* volatileContext) const;
    //  Определить по селекшенам, какой именно вариант пайплайны мы должны
    //  использовать
    uint32_t _getPipelineVariant(
                const TechniqueVolatileContext* volatileContext) const noexcept;

  private:
    Device& _device;

    std::string _debugName;

    Ref<TechniqueConfigurator> _configurator;
    ConstRef<TechniqueConfiguration> _configuration;
    const TechniqueConfiguration::DescriptorSet* _volatileSetDescription;
    const TechniqueConfiguration::DescriptorSet* _staticSetDescription;

    std::vector<std::unique_ptr<SelectionImpl>> _selections;
    size_t _lastProcessedSelectionsRevision;

    std::vector<std::unique_ptr<TechniqueResourceImpl>> _resources;
    size_t _resourcesRevision;

    using UniformBlocks = std::vector<std::unique_ptr<TechniqueUniformBlock>>;
    UniformBlocks _uniformBlocks;

    std::vector<std::unique_ptr<UniformVariableImpl>> _uniforms;

    //  Статик сет использует ленивую инициализацию, но сам используется в
    //  константных методах. Для защиты в многопоточном рендере используем
    //  мьютекс на спинлоке
    mutable Ref<DescriptorPool> _staticPool;
    mutable Ref<DescriptorSet> _staticSet;
    mutable size_t _lastProcessedResourcesRevision;
    mutable SpinLock _staticSetMutex;
  };

  inline Device& Technique::device() const noexcept
  {
    return _device;
  }

  inline TechniqueConfigurator& Technique::configurator() const noexcept
  {
    return *_configurator;
  }

  inline const std::string& Technique::debugName() const noexcept
  {
    return _debugName;
  }
}