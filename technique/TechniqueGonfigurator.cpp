#include <stdexcept>

#include <technique/ConfigurationBuildContext.h>
#include <technique/Technique.h>
#include <technique/TechniqueGonfigurator.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>

using namespace mt;

TechniqueGonfigurator::TechniqueGonfigurator( Device& device,
                                              const char* debugName) :
  _device(device),
  _debugName(debugName)
{
}

void TechniqueGonfigurator::propogateConfiguration() noexcept
{
  try
  {
    std::lock_guard lock(_observersMutex);
    for(Technique* observer : _observers)
    {
      observer->updateConfiguration();
    }
  }
  catch(std::exception& error)
  {
    Log::error() << _debugName << ": unable to propagate configuration: " << error.what();
    Abort("TechniqueConfigurator :: Unable to propagate configuration");
  }
}

void TechniqueGonfigurator::rebuildConfiguration()
{
  rebuildOnlyConfiguration();
  propogateConfiguration();
}

void TechniqueGonfigurator::rebuildOnlyConfiguration()
{
  ConfigurationBuildContext context{};
  context.configuratorName = _debugName;
  context.device = &_device;
  context.configuration = Ref(new TechniqueGonfiguration);
  context.configuration->volatileUniformBuffersSize = 0;

  _createSelections(context);
  _createPasses(context);
  _createLayouts(context);
  _createPipelines(context);

  {
    std::lock_guard lock(_configurationMutex);
    _configuration = context.configuration;
  }
}

void TechniqueGonfigurator::_createSelections(
                                      ConfigurationBuildContext& context) const
{
  // Проверим, нет ли дубликатов
  for (size_t i = 0; i < _selections.size(); i++)
  {
    for (size_t j = i + 1; j < _selections.size(); j++)
    {
      if (_selections[i].name == _selections[j].name)
      {
        throw std::runtime_error(_debugName + ": duplicate selection " + _selections[i].name);
      }
    }
  }

  context.configuration->selections.reserve(_selections.size());
  for(const SelectionDefine& selection : _selections)
  {
    if (selection.valueVariants.empty())
    {
      throw std::runtime_error(_debugName + ": selection " + selection.name + " doesn't have value variants");
    }

    context.configuration->selections.emplace_back();
    context.configuration->selections.back().name = selection.name;
    context.configuration->selections.back().valueVariants =
                                                        selection.valueVariants;
    context.configuration->selections.back().weights.reserve(_passes.size());
  }
}

void TechniqueGonfigurator::_createPasses(
                                      ConfigurationBuildContext& context) const
{
  if (_passes.empty())
  {
    throw std::runtime_error(_debugName + ": there aren't any passes");
  }
  context.configuration->_passes.reserve(_passes.size());
  context.shaders.reserve(_passes.size());

  for(uint32_t i = 0; i < _passes.size(); i++)
  {
    context.configuration->_passes.emplace_back();
    context.currentPassIndex = i;
    context.currentPass = &context.configuration->_passes.back();
    _passes[i]->processShaders(context);
  }
}

void TechniqueGonfigurator::_createLayouts(
                                      ConfigurationBuildContext& context) const
{
  // Пройдемся по всем ресурсам и рассортирруем их по сетам
  using SetBindings = std::vector<VkDescriptorSetLayoutBinding>;
  std::array<SetBindings, maxDescriptorSetIndex + 1> bindings;
  for(TechniqueGonfiguration::Resource& resource :
                                              context.configuration->resources)
  {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = resource.bindingIndex;
    binding.descriptorType = resource.type;
    binding.descriptorCount = resource.count;
    binding.stageFlags = resource.shaderStages;

    uint32_t setIndex = uint32_t(resource.set);
    bindings[setIndex].push_back(binding);
  }

  // Теперь можно создать лэйауты для сетов
  std::array< ConstRef<DescriptorSetLayout>,
              maxDescriptorSetIndex + 1> setLayouts;
  for(TechniqueGonfiguration::DescriptorSet& set :
                                          context.configuration->descriptorSets)
  {
    uint32_t setIndex = uint32_t(set.type);
    set.layout = ConstRef(new DescriptorSetLayout(_device,
                                                  bindings[setIndex]));
    setLayouts[setIndex] = set.layout;
  }

  // Создаем лэйаут пайплайна. Но сначала досоздаем лэйауты пустых сетов
  for(ConstRef<DescriptorSetLayout>& setLayout : setLayouts)
  {
    if(setLayout == nullptr)
    {
      setLayout = ConstRef(new DescriptorSetLayout(_device, {}));
    }
  }
  context.configuration->pipelineLayout =
                            ConstRef(new PipelineLayout( _device, setLayouts));
}

void TechniqueGonfigurator::_createPipelines(
                                      ConfigurationBuildContext& context) const
{
  for (uint32_t i = 0; i < _passes.size(); i++)
  {
    context.currentPassIndex = i;
    context.currentPass = &context.configuration->_passes[i];
    _passes[i]->createPipelines(context);
  }
}
