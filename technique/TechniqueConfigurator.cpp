#include <stdexcept>

#include <spirv_reflect.h>

#include <technique/DescriptorSetType.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Abort.h>
#include <util/Log.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/pipeline/ShaderModule.h>

using namespace mt;

TechniqueConfigurator::TechniqueConfigurator( Device& device,
                                              const char* debugName) noexcept :
  _device(device),
  _debugName(debugName),
  _pipelineType(AbstractPipeline::GRAPHIC_PIPELINE),
  _topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
  _rasterizationState{},
  _depthStencilState{},
  _blendingState{}
{
  _rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  _rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  _rasterizationState.lineWidth = 1;

  _depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  for(VkPipelineColorBlendAttachmentState& attachment : _attachmentsBlending)
  {
    attachment = VkPipelineColorBlendAttachmentState{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
  }
  _blendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  _blendingState.attachmentCount = 1;
  _blendingState.pAttachments = _attachmentsBlending.data();
}

void TechniqueConfigurator::_propogateConfiguration() noexcept
{
  try
  {
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

void TechniqueConfigurator::rebuildConfiguration()
{
  try
  {
    _configuration = Ref(new TechniqueConfiguration);

    _configuration->pipelineType = _pipelineType;
    if( _pipelineType == AbstractPipeline::GRAPHIC_PIPELINE &&
        !_frameBufferFormat.has_value())
    {
      throw std::runtime_error(_debugName + ": pipeline type is GRAPHIC_PIPELINE but the frame buffer format is empty");
    }

    if (_shaders.empty())
    {
      Log::warning() << _debugName << ": attempt to create a configuration without shaders";
      return;
    }

    ConfigurationBuildContext buildContext{};

    _processSelections(buildContext);

    if(_selections.empty()) _processShadersOneVariant(buildContext);
    else
    {
      _processShadersSeveralVariants( buildContext,
                                      uint32_t(_selections.size() - 1));
    }

    _createLayouts(buildContext);
    _createPipelines(buildContext);
  }
  catch(...)
  {
    _configuration.reset();
    throw;
  }

  _propogateConfiguration();
}

void TechniqueConfigurator::_processSelections(
                                        ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);

  // Для начала проверим, нет ли дубликатов
  for(size_t i = 0; i < _selections.size(); i++)
  {
    for(size_t j = i + 1; j < _selections.size(); j++)
    {
      if(_selections[i].name == _selections[j].name)
      {
        throw std::runtime_error(_debugName + ": duplicate selection " + _selections[i].name);
      }
    }
  }

  //  Посчитаем все варианты селекшенов, скопируем их в конфигурацию и
  //  посчитаем их веса
  buildContext.variantsNumber = 1;
  if(!_selections.empty())
  {
    buildContext.currentDefines.resize(_selections.size());

    _configuration->selections.reserve(_selections.size());
    for(const SelectionDefine& selection : _selections)
    {
      if(selection.valueVariants.empty())
      {
        throw std::runtime_error(_debugName + ": selection " + selection.name + " doesn't have value variants");
      }

      _configuration->selections.push_back(
                    TechniqueConfiguration::SelectionDefine{
                                                  selection.name,
                                                  selection.valueVariants,
                                                  buildContext.variantsNumber});
      buildContext.variantsNumber *= uint32_t(selection.valueVariants.size());
    }
  }

  //  Выделим место под разные варианты шейдерных модулей и пайплайнов
  buildContext.shaders.resize(buildContext.variantsNumber);
  if (_pipelineType == AbstractPipeline::COMPUTE_PIPELINE)
  {
    Abort("Not implemented");
  }
  else
  {
    _configuration->graphicPipelineVariants.resize(buildContext.variantsNumber);
  }
}

// Это рекурсивная функция, пребирающая все варианты селекшенов
void TechniqueConfigurator::_processShadersSeveralVariants(
                                  ConfigurationBuildContext& buildContext,
                                  uint32_t selectionIndex)
{
  const SelectionDefine& selection = _selections[selectionIndex];
  for(const std::string& selectionValue : selection.valueVariants)
  {
    buildContext.currentDefines[selectionIndex].name = selection.name.c_str();
    buildContext.currentDefines[selectionIndex].value = selectionValue.c_str();

    if(selectionIndex == 0)
    {
      // Мы обрабатываем последний селекшен из списка, пора переходить к шейдерам
      _processShadersOneVariant(buildContext);
      buildContext.currentVariantIndex++;
    }
    else
    {
      // Это ещё не последний селекшен, переходим к следующему из списка
      _processShadersSeveralVariants(buildContext, selectionIndex - 1);
    }
  }
}

void TechniqueConfigurator::_processShadersOneVariant(
                                      ConfigurationBuildContext& buildContext)
{
  for (const ShaderInfo& shader : _shaders)
  {
    _processShader(shader, buildContext);
  }
}

void TechniqueConfigurator::_processShader(
                                        const ShaderInfo& shaderRecord,
                                        ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);

  ShaderSet& shaderSet = buildContext.shaders[buildContext.currentVariantIndex];

  // Проверяем, что нет дублирования шейдерных стадий
  for(const ShaderModuleInfo& shader : shaderSet)
  {
    if(shader.stage == shaderRecord.stage)
    {
      throw std::runtime_error(_debugName + ": duplicate shader stages: " + shaderRecord.filename.c_str());
    }
  }

  // Получаем spirv код
  std::vector<uint32_t> spirData =
                      ShaderCompilator::compile(shaderRecord.filename.c_str(),
                                                shaderRecord.stage,
                                                buildContext.currentDefines);

  // Парсим spirv код
  spv_reflect::ShaderModule reflection( spirData.size() * sizeof(spirData[0]),
                                        spirData.data());
  if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
  {
    throw std::runtime_error(_debugName + ": unable to process SPIRV data: " + shaderRecord.filename.c_str());
  }
  const SpvReflectShaderModule& reflectModule = reflection.GetShaderModule();

  // Ищем нужные нам данные в рефлексии
  _processShaderReflection(shaderRecord, reflectModule);

  // Создаем шейдерный модуль
  std::unique_ptr<ShaderModule> newShaderModule(
                                    new ShaderModule(
                                                _device,
                                                spirData,
                                                shaderRecord.filename.c_str()));
  shaderSet.push_back(ShaderModuleInfo{ std::move(newShaderModule),
                                        shaderRecord.stage});
}

void TechniqueConfigurator::_processShaderReflection(
                                      const ShaderInfo& shaderRecord,
                                      const SpvReflectShaderModule& reflection)
{
  for(uint32_t iSet = 0; iSet < reflection.descriptor_set_count; iSet++)
  {
    const SpvReflectDescriptorSet& reflectedSet =
                                              reflection.descriptor_sets[iSet];
    uint32_t setIndex = reflectedSet.set;
    if(setIndex > maxDescriptorSetIndex)
    {
      throw std::runtime_error(_debugName + ": descriptor set with the wrong set index: " + shaderRecord.filename);
    }

    DescriptorSetType setType = DescriptorSetType(setIndex);
    TechniqueConfiguration::DescriptorSet& set = _getOrCreateSet(setType);
    _processBindings(shaderRecord, set, reflectedSet);
  }
}

TechniqueConfiguration::DescriptorSet&
                  TechniqueConfigurator::_getOrCreateSet(DescriptorSetType type)
{
  for(TechniqueConfiguration::DescriptorSet& set :
                                                _configuration->descriptorSets)
  {
    if(set.type == type) return set;
  }

  _configuration->descriptorSets.push_back({.type = type});
  return _configuration->descriptorSets.back();
}

static VkPipelineStageFlagBits getPipelineStage(
                                              VkShaderStageFlagBits shaderStage)
{
  switch(shaderStage)
  {
  case VK_SHADER_STAGE_VERTEX_BIT:
    return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
  case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
  case VK_SHADER_STAGE_GEOMETRY_BIT:
    return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  case VK_SHADER_STAGE_COMPUTE_BIT:
    return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
  case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
  case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
  case VK_SHADER_STAGE_MISS_BIT_KHR:
  case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
  case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
    return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  case VK_SHADER_STAGE_TASK_BIT_EXT:
    return VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
  case VK_SHADER_STAGE_MESH_BIT_EXT:
    return VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
  default:
    Abort("Unsupportet shader stage");
  }
}

void TechniqueConfigurator::_processBindings(
                                    const ShaderInfo& shaderRecord,
                                    TechniqueConfiguration::DescriptorSet& set,
                                    const SpvReflectDescriptorSet& reflectedSet)
{
  for(uint32_t iBinding = 0; iBinding < reflectedSet.binding_count; iBinding++)
  {
    const SpvReflectDescriptorBinding* reflectedBinding =
                                                reflectedSet.bindings[iBinding];
    MT_ASSERT(reflectedBinding != nullptr);

    // Для начала собираем общие данные о биндинге из рефлексии
    TechniqueConfiguration::Resource newResource;
    newResource.type = VkDescriptorType(reflectedBinding->descriptor_type);
    newResource.set = set.type;
    newResource.bindingIndex = reflectedBinding->binding;
    newResource.shaderStages = shaderRecord.stage;
    newResource.pipelineStages = getPipelineStage(shaderRecord.stage);
    newResource.name = reflectedBinding->name;
    newResource.writeAccess =
              reflectedBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_UAV;
    newResource.count = reflectedBinding->count;
    if( newResource.count > 1 &&
        newResource.type != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
        newResource.type != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
    {
      throw std::runtime_error(_debugName + ": " + shaderRecord.filename + ": only images arrays are supported.");
    }

    // Смотрим, возможно этот биндинг уже появлялся на других стадиях
    bool itIsNewResource = true;
    for(TechniqueConfiguration::Resource& resource : _configuration->resources)
    {
      if( resource.set == newResource.set &&
          resource.bindingIndex == newResource.bindingIndex)
      {
        // Проверяем, что биндинги совместимы
        if( resource.type != newResource.type ||
            resource.name != newResource.name ||
            resource.writeAccess != newResource.writeAccess ||
            resource.count != newResource.count)
        {
          Log::warning() << _debugName << ": binding conflict: " << shaderRecord.filename << " set: " << uint32_t(resource.set) << " binding: " << resource.bindingIndex;
          throw std::runtime_error(_debugName + ": binding conflict: " + shaderRecord.filename);
        }
        // Биндинги совместимы, просто дополняем информацию
        resource.shaderStages |= newResource.shaderStages;
        resource.pipelineStages |= newResource.pipelineStages;
        itIsNewResource = false;
        break;
      }
    }
    // Если не нашли такой же биндинг, то добавляем новый
    if(itIsNewResource)
    {
      _configuration->resources.push_back(newResource);
    }

    // Если это юниформ буффер - парсим его
    if(reflectedBinding->descriptor_type ==
                                    SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
      _processUniformBlock(shaderRecord,*reflectedBinding);
    }
  }
}

void TechniqueConfigurator::_processUniformBlock(
                          const ShaderInfo& shaderRecord,
                          const SpvReflectDescriptorBinding& reflectedBinding)
{
  const SpvReflectBlockVariable& block = reflectedBinding.block;

  // Для начала ищем, возможно буфер с таким биндингом уже встречался
  for(TechniqueConfiguration::UniformBuffer& buffer :
                                                _configuration->uniformBuffers)
  {
    if( uint32_t(buffer.set) == reflectedBinding.set &&
        buffer.binding == reflectedBinding.binding)
    {
      // Нашли буфер с таким же биндингом, проверяем имя и размер
      if( buffer.name != reflectedBinding.name ||
          buffer.size != block.size)
      {
        Log::warning() << _debugName << ": uniform buffer mismatch set:" << reflectedBinding.set << " binding: " << reflectedBinding.binding << " file:" << shaderRecord.filename;
        throw std::runtime_error(_debugName + ": uniform buffer mismatch: " + shaderRecord.filename);
      }
      return;
    }
  }

  // Этот буфер раньше не встречался, посмотрим что внутри
  TechniqueConfiguration::UniformBuffer newBuffer{};
  newBuffer.set = DescriptorSetType(reflectedBinding.set);
  newBuffer.binding = reflectedBinding.binding;
  newBuffer.name = reflectedBinding.name;
  newBuffer.size = block.size;
  //  Обходим всё содержимое и ищем куски, которые мы можем интерпретировать
  //  как параметры
  std::string prefix = newBuffer.name + ".";
  for(uint32_t iMember = 0; iMember < block.member_count; iMember++)
  {
    _parseUniformBlockMember( shaderRecord,
                              newBuffer,
                              block.members[iMember],
                              prefix,
                              0);
  }

  _configuration->uniformBuffers.push_back(newBuffer);
}

void TechniqueConfigurator::_parseUniformBlockMember(
                                const ShaderInfo& shaderRecord,
                                TechniqueConfiguration::UniformBuffer& target,
                                const SpvReflectBlockVariable& sourceMember,
                                std::string namePrefix,
                                uint32_t parentBlockOffset)
{
  std::string fullName = namePrefix + sourceMember.name;
  uint32_t blockOffset = parentBlockOffset + sourceMember.offset;
  SpvReflectTypeDescription* type = sourceMember.type_description;
  MT_ASSERT(type != nullptr);

  if(type->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
  {
    //  Если мы обнаружили структуру, то просто обходим её поля. Сама структура
    //  не может быть параметром эффекта
    std::string prefix = fullName + ".";
    for(uint32_t iMember = 0; iMember < sourceMember.member_count; iMember++)
    {
      _parseUniformBlockMember( shaderRecord,
                                target,
                                sourceMember.members[iMember],
                                prefix,
                                blockOffset);
    }
    return;
  }

  // Этот блок - не структура. Парсим его как отдельную униформ переменную
  TechniqueConfiguration::UniformVariable newUniform{};
  newUniform.shortName = sourceMember.name;
  newUniform.fullName = fullName;
  newUniform.offsetInBuffer = blockOffset;
  newUniform.size = sourceMember.size;

  // Определяем базовый тип
  if(type->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
  {
    newUniform.baseType = TechniqueConfiguration::BOOL_TYPE;
  }
  else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT)
  {
    newUniform.baseType = TechniqueConfiguration::INT_TYPE;
  }
  else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
  {
    newUniform.baseType = TechniqueConfiguration::FLOAT_TYPE;
  }
  else newUniform.baseType = TechniqueConfiguration::UNKNOWN_TYPE;

  // Это вектор или массив векторов
  if(type->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
  {
    newUniform.isVector = true;
    newUniform.vectorSize = type->traits.numeric.vector.component_count;
  }

  // Это матрица или массив матриц
  if(type->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
  {
    MT_ASSERT(!newUniform.isVector);
    newUniform.isMatrix = true;
    newUniform.matrixSize = glm::uvec2(
                                    type->traits.numeric.matrix.column_count,
                                    type->traits.numeric.matrix.row_count);
  }

  // Проверяем, не массив ли это
  if(type->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
  {
    // Поддерживаем только одномерные массивы. Многомерные интерпретируем
    // просто как набор байтов
    if(type->traits.array.dims_count == 1)
    {
      newUniform.isArray = true;
      newUniform.arraySize = type->traits.array.dims[0];
      newUniform.arrayStride = type->traits.array.stride;
    }
    else
    {
      newUniform.baseType = TechniqueConfiguration::UNKNOWN_TYPE;
      newUniform.isVector = false;
      newUniform.isMatrix = false;
    }
  }

  target.variables.push_back(newUniform);
}

void TechniqueConfigurator::_createLayouts(
                                        ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);

  // Пройдемся по всем ресурсам и рассортирруем их по сетам
  using SetBindings = std::vector<VkDescriptorSetLayoutBinding>;
  std::array<SetBindings, maxDescriptorSetIndex + 1> bindings;
  for(TechniqueConfiguration::Resource& resource : _configuration->resources)
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
  for(TechniqueConfiguration::DescriptorSet& set :
                                                _configuration->descriptorSets)
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
  _configuration->pipelineLayout = ConstRef(new PipelineLayout( _device,
                                                                setLayouts));
}

void TechniqueConfigurator::_createPipelines(
                                      ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);
  MT_ASSERT(_configuration->pipelineLayout != nullptr)

  for(uint32_t variant = 0; variant < buildContext.variantsNumber; variant++)
  {
    //  Формируем список шейдерных модулей, из которых состоит пайплайн
    ShaderSet& shaderSet = buildContext.shaders[variant];
    std::vector<AbstractPipeline::ShaderInfo> shaders;
    for(ShaderModuleInfo& shader : shaderSet)
    {
      shaders.push_back(AbstractPipeline::ShaderInfo{
                                          .module = shader.shaderModule.get(),
                                          .stage = shader.stage,
                                          .entryPoint = "main"});
    }

    //  Создаем сам пайплайн
    if (_pipelineType == AbstractPipeline::COMPUTE_PIPELINE)
    {
      Abort("Not implemented");
    }
    else
    {
      _configuration->graphicPipelineVariants[variant] =
                              ConstRef(new GraphicPipeline(
                                              *_frameBufferFormat,
                                              shaders,
                                              _topology,
                                              _rasterizationState,
                                              _depthStencilState,
                                              _blendingState,
                                              *_configuration->pipelineLayout));
    }
  }
}
