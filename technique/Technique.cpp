#include <mutex>
#include <stdexcept>

#include <technique/Technique.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/Device.h>

using namespace mt;

Technique::Technique(TechniqueConfigurator& configurator) :
  _device(configurator.device()),
  _debugName(configurator.debugName()),
  _configurator(&configurator),
  _volatileSetDescription(nullptr),
  _staticSetDescription(nullptr),
  _lastProcessedSelectionsRevision(0),
  _resourcesRevision(0),
  _staticSetComplete(true),
  _lastProcessedResourcesRevision(0)
{
  _configurator->addObserver(*this);
  updateConfiguration();
}

Technique::~Technique() noexcept
{
  _configurator->removeObserver(*this);
}

void Technique::updateConfiguration()
{
  ConstRef<TechniqueConfiguration> newConfiguration =
                                                _configurator->configuration();
  if(newConfiguration == nullptr) return;

  // Пересоздаем юниформ блоки
  UniformBlocks newUniformBlocks;
  if(newConfiguration != nullptr)
  {
    for(const TechniqueConfiguration::UniformBuffer& description :
                                            newConfiguration->uniformBuffers)
    {
      newUniformBlocks.emplace_back(
                                new TechniqueUniformBlock(description,
                                                          *this,
                                                          _resourcesRevision));
    }
  }

  //  Чистим ресурсы с дефолтными значениями, на случай, если они не прописаны в
  //  новой конфигурации
  for (std::unique_ptr<ResourceBindingImpl>& resource : _resources)
  {
    if(resource->isDefault()) resource->clear();
  }

  try
  {
    // Добавляем дефолтные сэмплеры
    if(newConfiguration != nullptr)
    {
      for(const TechniqueConfiguration::DefaultSampler& sampler :
                                            newConfiguration->defaultSamplers)
      {
        ResourceBindingImpl& samplerResource =
                      static_cast<ResourceBindingImpl&>(
                                    getOrCreateResourceBinding(
                                                  sampler.resourceName.c_str()));
        if(samplerResource.isDefault())
        {
          samplerResource.setSamplerAsDefault(sampler.defaultSampler.get());
        }
      }
    }

    // Биндим дочерние компоненты
    for (std::unique_ptr<SelectionImpl>& selection : _selections)
    {
      selection->setConfiguration(newConfiguration.get());
    }

    for (std::unique_ptr<ResourceBindingImpl>& resource : _resources)
    {
      resource->setConfiguration(newConfiguration.get());
    }
    _resourcesRevision++;

    for (std::unique_ptr<UniformVariableImpl>& uniform : _uniforms)
    {
      uniform->setConfiguration(newConfiguration.get(), newUniformBlocks);
    }

    for (std::unique_ptr<TechniquePassImpl>& pass : _passes)
    {
      pass->setConfiguration(newConfiguration.get());
    }
  }
  catch(std::exception& error)
  {
    // Откатить технику назад не получится, поэтому лучше упадем
    Log::error() << _debugName << ": unable to update configuration: " << error.what();
    Abort("Unable to update technique configuration");
  }

  // Ищем описания дескриптер сетов
  _volatileSetDescription = nullptr;
  _staticSetDescription = nullptr;
  if(newConfiguration != nullptr)
  {
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
  }

  _uniformBlocks = std::move(newUniformBlocks);
  _configuration = ConstRef(newConfiguration);
}

bool Technique::isReady(const TechniqueVolatileContext* volatileContext) const
{
  if(_configuration == nullptr) return false;

  // Проверка ресурсов в статик сете
  {
    std::lock_guard lock(_staticSetMutex);
    if (_lastProcessedResourcesRevision == _resourcesRevision)
    {
      //  Если ревизии совпадают, то мы можем доверять _staticSetComplete
      if(!_staticSetComplete) return false;
    }
    else
    {
      //  Ревизии не совпадают, статик сет ещё не собран, доверять
      //  _staticSetComplete нельзя
      if(!_checkSetReady(DescriptorSetType::STATIC)) return false;
    }
  }

  // Проверка волатильных ресурсов
  if(volatileContext == nullptr)
  {
    //  Волатильный контекст не используется, поэтому просто посчитаем,
    //    сколько подключено непустых ресурсов
    return _checkSetReady(DescriptorSetType::VOLATILE);
  }
  //  Используется волатильный контекст. Узнаем количество прибинженных ресурсов
  //  из него
  return volatileContext->resourcesBinded >=
              _configuration->resourcesCount[(int)DescriptorSetType::VOLATILE];
}

bool Technique::bindGraphic(
                          CommandProducerGraphic& producer,
                          const TechniquePass& pass,
                          const TechniqueVolatileContext* volatileContext) const
{
  if(_configuration == nullptr)
  {
    Log::warning() << _debugName << ": configuration is invalid or not builded";
    return false;
  }

  uint32_t passIndex = static_cast<const TechniquePassImpl&>(pass).passIndex();
  if(passIndex == TechniquePassImpl::NOT_PASS_INDEX)
  {
    Log::warning() << _debugName << ": pass " << pass.name() << " is not found";
    return false;
  }
  if(_configuration->_passes[passIndex].pipelineType != AbstractPipeline::GRAPHIC_PIPELINE)
  {
    Log::warning() << _debugName << ": technique is not graphic";
    return false;
  }

  _updateStaticSet(producer);
  _bindDescriptorsGraphic(producer, volatileContext);

  uint32_t pipelineVariant = _getPipelineVariant(volatileContext, passIndex);
  MT_ASSERT(pipelineVariant < _configuration->_passes[passIndex].graphicPipelineVariants.size());

  producer.setGraphicPipeline(
                  *_configuration->_passes[passIndex].
                                      graphicPipelineVariants[pipelineVariant]);
  return true;
}

void Technique::_updateStaticSet(CommandProducerTransfer& commandProducer) const
{
  std::lock_guard lock(_staticSetMutex);

  if (_staticSetDescription == nullptr)
  {
    _staticSetComplete = true;
    return;
  }

  // Если ревизия ресурсов изменилась, значит в статик сете поменялся
  // кто-то из ресурсов. Надо пересоздавать статик сет
  if (_lastProcessedResourcesRevision != _resourcesRevision)
  {
    _staticSet.reset();
    _staticPool.reset();
    _lastProcessedResourcesRevision = _resourcesRevision;
  }
  // Создаем новый сет, если он ещё не был создан
  if(_staticSet == nullptr)
  {
    _staticPool = Ref(new DescriptorPool(
                            _device,
                            _staticSetDescription->layout->descriptorCounter(),
                            1,
                            DescriptorPool::STATIC_POOL));
    _staticSet = _staticPool->allocateSet(*_staticSetDescription->layout);
    _bindResources(*_staticSet, DescriptorSetType::STATIC, commandProducer);
    _staticSetComplete = _checkSetReady(DescriptorSetType::STATIC);
  }
}

bool Technique::_checkSetReady(DescriptorSetType setType) const noexcept
{
  if(_configuration == 0) return false;

  //  Подсчитываем, сколько всего у нас есть непустых ресурсов, которые биндятся
  //  к этому сэту
  int resourcesCount = 0;
  for(const std::unique_ptr<ResourceBindingImpl>& resource : _resources)
  {
    const TechniqueConfiguration::Resource* description =
                                                        resource->description();
    if( description != nullptr &&
        description->set == setType &&
        !resource->empty())
    {
      resourcesCount++;
    }
  }

  //  То же самое для юниформ блоков
  for (const std::unique_ptr<TechniqueUniformBlock>& uniformBlock :
                                                                _uniformBlocks)
  {
    if(uniformBlock->description().set == setType) resourcesCount++;
  }

  return resourcesCount == _configuration->resourcesCount[(int)setType];
}

void Technique::_bindResources( DescriptorSet& descriptorSet,
                                DescriptorSetType setType,
                                CommandProducerTransfer& commandProducer) const
{
  for(const std::unique_ptr<ResourceBindingImpl>& resource : _resources)
  {
    if( resource->description() != nullptr &&
        resource->description()->set == setType)
    {
      resource->bindToDescriptorSet(descriptorSet);
    }
  }

  for (const std::unique_ptr<TechniqueUniformBlock>& uniformBlock :
                                                                _uniformBlocks)
  {
    if(uniformBlock->description().set == setType)
    {
      uniformBlock->bindToDescriptorSet(descriptorSet,
                                        commandProducer,
                                        nullptr);
    }
  }

  descriptorSet.finalize();
}

void Technique::_bindDescriptorsGraphic(
                          CommandProducerGraphic& producer,
                          const TechniqueVolatileContext* volatileContext) const
{
  // Сначала биндим волатильный сет
  if(_volatileSetDescription != nullptr)
  {
    if (volatileContext != nullptr)
    {
      //  Сет был создан ранее при создании контекста
      MT_ASSERT(volatileContext->descriptorSet != nullptr);
      //  Ресурсы уже были прибинжены ранее, остались только униформ блоки
      for (const std::unique_ptr<TechniqueUniformBlock>& uniformBlock :
                                                                _uniformBlocks)
      {
        if(uniformBlock->description().set == DescriptorSetType::VOLATILE)
        {
          uniformBlock->bindToDescriptorSet(*volatileContext->descriptorSet,
                                            producer,
                                            volatileContext);
        }
      }
      volatileContext->descriptorSet->finalize();
      producer.bindDescriptorSetGraphic(*volatileContext->descriptorSet,
                                        uint32_t(DescriptorSetType::VOLATILE),
                                        *_configuration->pipelineLayout);
    }
    else
    {
      // Волатильного контекста нет, поэтому создадим и заполним новый сет
      Ref<DescriptorSet> volatileSet =
        producer.descriptorPool().allocateSet(*_volatileSetDescription->layout);
      _bindResources(*volatileSet, DescriptorSetType::VOLATILE, producer);

      producer.bindDescriptorSetGraphic(*volatileSet,
                                        uint32_t(DescriptorSetType::VOLATILE),
                                        *_configuration->pipelineLayout);
    }
  }

  // Биндим статический сет
  if (_staticSetDescription != nullptr)
  {
    std::lock_guard lock(_staticSetMutex);
    MT_ASSERT(_staticSet != nullptr);
    producer.bindDescriptorSetGraphic(*_staticSet,
                                      uint32_t(DescriptorSetType::STATIC),
                                      *_configuration->pipelineLayout);
  }
}

uint32_t Technique::_getPipelineVariant(
                const TechniqueVolatileContext* volatileContext,
                uint32_t passIndex) const noexcept
{
  uint32_t pipelineVariant = 0;
  if(volatileContext != nullptr)
  {
    // Смотрим, что записали селекшены в контекст
    for (size_t i = 0; i < _selections.size(); i++)
    {
      uint32_t valueIndex = volatileContext->selectionsValues[i];
      pipelineVariant += _selections[i]->valueWeight(valueIndex, passIndex);
    }
  }
  else
  {
    // Волатильного контекста нет - опрашиваем селекшены напрямую
    for (const std::unique_ptr<SelectionImpl>& selection : _selections)
    {
      pipelineVariant += selection->valueWeight(passIndex);
    }
  }
  return pipelineVariant;
}

TechniqueVolatileContext Technique::createVolatileContext(
                                                CommandProducer& producer) const
{
  const DescriptorSetLayout* volatileSetLayout = nullptr;
  if (_volatileSetDescription != nullptr)
  {
    volatileSetLayout = _volatileSetDescription->layout.get();
  }

  size_t uniformDataSize = 0;
  if(_configuration != nullptr)
  {
    uniformDataSize = _configuration->volatileUniformBuffersSize;
  }

  TechniqueVolatileContext context( producer,
                                    volatileSetLayout,
                                    _selections.size(),
                                    uniformDataSize);

  //  Копируем установленные значения селекшенов в контекст
  for(size_t i = 0; i < _selections.size(); i++)
  {
    context.selectionsValues[i] = _selections[i]->valueIndex();
  }

  //  Биндим установленные ресурсы
  if(context.descriptorSet != nullptr)
  {
    for (const std::unique_ptr<ResourceBindingImpl>& resource : _resources)
    {
      if (resource->description() != nullptr &&
          !resource->empty() &&
          resource->description()->set == DescriptorSetType::VOLATILE)
      {
        resource->bindToDescriptorSet(*context.descriptorSet);
        context.resourcesBinded++;
      }
    }
  }

  //  Копируем данные униформ буферов
  for(const std::unique_ptr<TechniqueUniformBlock>& block : _uniformBlocks)
  {
    if(block->description().set == DescriptorSetType::VOLATILE)
    {
      void* dstData = (char*)context.uniformData +
                                    block->description().volatileContextOffset;
      block->copyDataTo(dstData);
      context.resourcesBinded++;
    }
  }

  return context;
}

void Technique::unbindGraphic(CommandProducerGraphic& producer) const noexcept
{
  if(_volatileSetDescription != nullptr)
  {
    producer.unbindDescriptorSetGraphic(uint32_t(DescriptorSetType::VOLATILE));
  }
  if (_staticSetDescription != nullptr)
  {
    producer.unbindDescriptorSetGraphic(uint32_t(DescriptorSetType::STATIC));
  }
}

Selection& Technique::getOrCreateSelection(const char* selectionName)
{
  for(std::unique_ptr<SelectionImpl>& selection : _selections)
  {
    if(selection->name() == selectionName) return *selection;
  }
  _selections.push_back(std::make_unique<SelectionImpl>(
                                                selectionName,
                                                *this,
                                                _configuration.get(),
                                                uint32_t(_selections.size())));
  return *_selections.back();
}

ResourceBinding& Technique::getOrCreateResourceBinding(const char* resourceName)
{
  for(std::unique_ptr<ResourceBindingImpl>& resource : _resources)
  {
    if(resource->name() == resourceName) return *resource;
  }
  _resources.push_back(std::make_unique<ResourceBindingImpl>(
                                                        resourceName,
                                                        *this,
                                                        _resourcesRevision,
                                                        _configuration.get()));
  _resourcesRevision++;
  return *_resources.back();
}

UniformVariable& Technique::getOrCreateUniform(const char* uniformFullName)
{
  for(std::unique_ptr<UniformVariableImpl>& unifrom : _uniforms)
  {
    if(unifrom->name() == uniformFullName) return *unifrom;
  }
  _uniforms.push_back(std::make_unique<UniformVariableImpl>(
                                                        uniformFullName,
                                                        *this,
                                                        _configuration.get(),
                                                        _uniformBlocks));
  return *_uniforms.back();
}

TechniquePass& Technique::getOrCreatePass(const char* passName)
{
  for(std::unique_ptr<TechniquePassImpl>& pass : _passes)
  {
    if(pass->name() == passName) return *pass;
  }
  _passes.push_back(std::make_unique<TechniquePassImpl>(passName,
                                                        _configuration.get()));
  return *_passes.back();
}
