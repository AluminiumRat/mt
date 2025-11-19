#pragma once

#include <memory>
#include <string>
#include <vector>

#include <technique/Selection.h>
#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/TechniqueResource.h>
#include <technique/TechniqueUniformBlock.h>
#include <technique/UniformVariable.h>
#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/DescriptorPool.h>
#include <vkr/DescriptorSet.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  class Technique : public RefCounter
  {
  public:
    Technique(Device& device, const char* debugName = "Technique");
    Technique(const Technique&) = delete;
    Technique& operator = (const Technique&) = delete;
  protected:
    virtual ~Technique() noexcept = default;
  public:

    inline Device& device() const noexcept;

    inline TechniqueConfigurator& configurator() const noexcept;

    //  Подключить пайплайн и все необходимые ресурсы к продюсеро
    //  Возвращает false, если по какой-то причине подключить технику не
    //    получается. В этом случае причина отказа пишется в лог в виде
    //    Warning сообщения
    //  ВНИМАНИЕ!!! Нельзя одновременно подключать несколько техник к одному
    //    продюсеру.
    bool bindGraphic(CommandProducerGraphic& producer);
    void unbindGraphic(CommandProducerGraphic& producer) noexcept;

    Selection& getOrCreateSelection(const char* selectionName);
    TechniqueResource& getOrCreateResource(const char* resourceName);
    UniformVariable& getOrCreateUniform(const char* uniformFullName);

    inline const std::string& debugName() const noexcept;

  private:
    void _checkConfiguration();
    void _checkResources() noexcept;
    void _bindDescriptorsGraphic(CommandProducerGraphic& producer);
    void _bindResources(DescriptorSet& descriptorSet,
                        DescriptorSetType setType);
    void _buildStaticSet();
    void _checkSelections() noexcept;

  private:
    Device& _device;
    std::string _debugName;
    bool _isBinded;
    Ref<TechniqueConfigurator> _configurator;
    ConstRef<TechniqueConfiguration> _configuration;
    const TechniqueConfiguration::DescriptorSet* _volatileSetDescription;
    const TechniqueConfiguration::DescriptorSet* _staticSetDescription;
    size_t _lastConfiguratorRevision;

    uint32_t _pipelineVariant;

    std::vector<std::unique_ptr<SelectionImpl>> _selections;
    size_t _selectionsRevision;
    size_t _lastProcessedSelectionsRevision;

    std::vector<std::unique_ptr<TechniqueResourceImpl>> _resources;
    size_t _resourcesRevision;
    size_t _lastProcessedResourcesRevision;

    using UniformBlocks = std::vector<std::unique_ptr<TechniqueUniformBlock>>;
    UniformBlocks _uniformBlocks;

    std::vector<std::unique_ptr<UniformVariableImpl>> _uniforms;

    Ref<DescriptorPool> _staticPool;
    Ref<DescriptorSet> _staticSet;
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