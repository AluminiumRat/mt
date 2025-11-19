#include <stdexcept>

#include <technique/Technique.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

Technique::Technique(Device& device, const char* debugName) :
  _device(device),
  _debugName(debugName),
  _isBinded(false),
  _configurator(new TechniqueConfigurator(
                                      device,
                                      (_debugName + "Configurator").c_str())),
  _volatileSetDescription(nullptr),
  _staticSetDescription(nullptr),
  _lastConfiguratorRevision(0),
  _pipelineVariant(0),
  _selectionsRevision(0),
  _lastProcessedSelectionsRevision(0),
  _resourcesRevision(0),
  _lastProcessedResourcesRevision(0)
{
}

bool Technique::bindGraphic(CommandProducerGraphic& producer)
{
  MT_ASSERT(!_isBinded);

  _checkConfiguration();
  if(_configuration == nullptr)
  {
    Log::warning() << _debugName << ": configuration is invalid";
    return false;
  }
  if(_configurator->pipelineType() != AbstractPipeline::GRAPHIC_PIPELINE)
  {
    Log::warning() << _debugName << ": technique is not graphic";
    return false;
  }

  for(std::unique_ptr<TechniqueUniformBlock>& uniformBlock : _uniformBlocks)
  {
    uniformBlock->update(producer);
  }

  _checkResources();
  _bindDescriptorsGraphic(producer);

  _checkSelections();
  MT_ASSERT(_pipelineVariant < _configuration->graphicPipelineVariants.size());

  producer.setGraphicPipeline(
                    *_configuration->graphicPipelineVariants[_pipelineVariant]);

  _isBinded = true;
  return true;
}

void Technique::_checkConfiguration()
{
  if(_lastConfiguratorRevision == _configurator->revision()) return;

  // Ревизия конфигурации поменялась, надо установить свежую конфигурацию
  _lastConfiguratorRevision = _configurator->revision();
  const TechniqueConfiguration* newConfiguration =
                                              _configurator->configuration();
  _volatileSetDescription = nullptr;
  _staticSetDescription = nullptr;

  // Заранее найдем нужные сеты
  for(const TechniqueConfiguration::DescriptorSet& setDescription :
                                            newConfiguration->descriptorSets)
  {
    switch(setDescription.type)
    {
    case DescriptorSetType::VOLATILE:
      _volatileSetDescription = &setDescription;
      break;
    case DescriptorSetType::STATIC:
      _staticSetDescription = &setDescription;
      break;
    }
  }

  UniformBlocks _newUniformBlocks;
  for(const TechniqueConfiguration::UniformBuffer& description :
                                            newConfiguration->uniformBuffers)
  {
    _newUniformBlocks.emplace_back(
                              new TechniqueUniformBlock(description, *this));
  }

  try
  {
    // Биндим дочерние компоненты
    for (std::unique_ptr<SelectionImpl>& selection : _selections)
    {
      selection->setConfiguration(newConfiguration);
    }
    _selectionsRevision++;

    for (std::unique_ptr<TechniqueResourceImpl>& resource : _resources)
    {
      resource->setConfiguration(newConfiguration);
    }
    _resourcesRevision++;
  }
  catch(std::exception& error)
  {
    // Откатить технику назад не получится, поэтому лучше упадем
    Log::error() << _debugName << ": unable to update configuration: " << error.what();
    Abort("Unable to update technique configuration");
  }

  _uniformBlocks = std::move(_newUniformBlocks);
  _configuration = ConstRef(newConfiguration);
}

void Technique::_checkResources() noexcept
{
  // Если ревизия изменилась, значит в статик сете поменялся кто-то из ресурсов
  // Надо пересоздавать статик сет
  if (_lastProcessedResourcesRevision != _resourcesRevision)
  {
    _staticSet.reset();
    _staticPool.reset();
    _lastProcessedResourcesRevision = _resourcesRevision;
  }
}

void Technique::_bindDescriptorsGraphic(CommandProducerGraphic& producer)
{
  // Волатильный сет всегда выделяем по новой
  if(_volatileSetDescription != nullptr)
  {
    Ref<DescriptorSet> volatileSet =
        producer.descriptorPool().allocateSet(*_volatileSetDescription->layout);
    _bindResources(*volatileSet, DescriptorSetType::VOLATILE);

    producer.bindDescriptorSetGraphic(*volatileSet,
                                      uint32_t(DescriptorSetType::VOLATILE),
                                      *_configuration->pipelineLayout);
  }

  if(_staticSetDescription != nullptr)
  {
    // Статик сет обновляется только по необходимости
    if(_staticSet == nullptr) _buildStaticSet();
    producer.bindDescriptorSetGraphic(*_staticSet,
                                      uint32_t(DescriptorSetType::STATIC),
                                      *_configuration->pipelineLayout);
  }
}

void Technique::_bindResources( DescriptorSet& descriptorSet,
                                DescriptorSetType setType)
{
  for(std::unique_ptr<TechniqueResourceImpl>& resource : _resources)
  {
    if( resource->description() != nullptr &&
        resource->description()->set == setType)
    {
      resource->bindToDescriptorSet(descriptorSet);
    }
  }

  for (std::unique_ptr<TechniqueUniformBlock>& uniformBlock : _uniformBlocks)
  {
    uniformBlock->bindToDescriptorSet(descriptorSet);
  }

  descriptorSet.finalize();
}

void Technique::_buildStaticSet()
{
  _staticPool = Ref(new DescriptorPool(
                            _device,
                            _staticSetDescription->layout->descriptorCounter(),
                            1,
                            DescriptorPool::STATIC_POOL));
  _staticSet = _staticPool->allocateSet(*_staticSetDescription->layout);
  _bindResources(*_staticSet, DescriptorSetType::STATIC);
}

void Technique::_checkSelections() noexcept
{
  if(_lastProcessedSelectionsRevision != _selectionsRevision)
  {
    _pipelineVariant = 0;
    for (std::unique_ptr<SelectionImpl>& selection : _selections)
    {
      _pipelineVariant += selection->valueWeight();
    }
    _lastProcessedSelectionsRevision = _selectionsRevision;
  }
}

void Technique::unbindGraphic(CommandProducerGraphic& producer) noexcept
{
  MT_ASSERT(_isBinded);
  if(_volatileSetDescription != nullptr)
  {
    producer.unbindDescriptorSetGraphic(uint32_t(DescriptorSetType::VOLATILE));
  }
  if (_staticSetDescription != nullptr)
  {
    producer.unbindDescriptorSetGraphic(uint32_t(DescriptorSetType::STATIC));
  }
  _isBinded = false;
}

Selection& Technique::getOrCreateSelection(const char* selectionName)
{
  for(std::unique_ptr<SelectionImpl>& selection : _selections)
  {
    if(selection->name() == selectionName) return *selection;
  }
  _selections.push_back(std::make_unique<SelectionImpl>(selectionName,
                                                        *this,
                                                        _selectionsRevision,
                                                        _configuration.get()));
  _selectionsRevision++;
  return *_selections.back();
}

TechniqueResource& Technique::getOrCreateResource(const char* resourceName)
{
  for(std::unique_ptr<TechniqueResourceImpl>& resource : _resources)
  {
    if(resource->name() == resourceName) return *resource;
  }
  _resources.push_back(std::make_unique<TechniqueResourceImpl>(
                                                        resourceName,
                                                        *this,
                                                        _resourcesRevision,
                                                        _configuration.get()));
  _resourcesRevision++;
  return *_resources.back();
}
