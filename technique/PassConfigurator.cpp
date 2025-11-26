#include <stdexcept>

#include <spirv_reflect.h>

#include <technique/ConfigurationBuildContext.h>
#include <technique/PassConfigurator.h>
#include <technique/TechniqueConfiguration.h>
#include <util/Abort.h>
#include <util/Log.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/pipeline/ShaderModule.h>

using namespace mt;

PassConfigurator::PassConfigurator(const char* name) :
  _name(name),
  _pipelineType(AbstractPipeline::GRAPHIC_PIPELINE),
  _topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
  _rasterizationState{},
  _depthStencilState{},
  _blendingState{}
{
  _rasterizationState.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  _rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  _rasterizationState.lineWidth = 1;

  _depthStencilState.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

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

void PassConfigurator::processShaders(ConfigurationBuildContext& context) const
{
  if( _pipelineType == AbstractPipeline::GRAPHIC_PIPELINE &&
      !_frameBufferFormat.has_value())
  {
    throw std::runtime_error(context.configuratorName + ":" + _name + ": pipeline type is GRAPHIC_PIPELINE but the frame buffer format is empty");
  }
  if (_shaders.empty())
  {
    throw std::runtime_error(context.configuratorName + ":" + _name + ": attempt to create a configuration without shaders");
  }

  context.currentPass->pipelineType = _pipelineType;
  context.currentPass->name = _name;

  _processSelections(context);

  context.currentVariantIndex = 0;
  if (_selections.empty()) _processShadersOneVariant(context);
  else
  {
    _processShadersSeveralVariants(uint32_t(_selections.size() - 1), context);
  }
}

static TechniqueConfiguration::SelectionDefine* findSelection(
                                            ConfigurationBuildContext& context,
                                            const std::string& selectionName)
{
  for (TechniqueConfiguration::SelectionDefine& selection :
                                              context.configuration->selections)
  {
    if(selection.name == selectionName) return &selection;
  }
  return nullptr;
}

void PassConfigurator::_processSelections(
                                      ConfigurationBuildContext& context) const
{
  // Проверим, нет ли дубликатов
  for (size_t i = 0; i < _selections.size(); i++)
  {
    for (size_t j = i + 1; j < _selections.size(); j++)
    {
      if (_selections[i] == _selections[j])
      {
        throw std::runtime_error(context.configuratorName + ":" + _name + ": duplicate selection " + _selections[i]);
      }
    }
  }

  //  Добавим во все результирующие селекшены дефолтный нулевой вес для этого
  //  прохода
  for(TechniqueConfiguration::SelectionDefine& selection :
                                              context.configuration->selections)
  {
    selection.weights.push_back(0);
  }

  //  Обходим селекшены, которые используются в этом проходе, посчитаем
  //  количество вариантов и веса селекшенов
  uint32_t variantsNumber = 1;
  if (!_selections.empty())
  {
    context.currentDefines.resize(_selections.size());

    for(const std::string& selectionName : _selections)
    {
      TechniqueConfiguration::SelectionDefine* selection =
                                          findSelection(context, selectionName);
      if(selection == nullptr)
      {
        throw std::runtime_error(context.configuratorName + ": " + _name + ": selection " + selectionName + " is used in the pass but it isn't defined in the technique");
      }
      selection->weights.back() = variantsNumber;
      variantsNumber *= uint32_t(selection->valueVariants.size());
    }
  }

  //  Выделим место под разные варианты шейдерных модулей и пайплайнов
  context.shaders[context.currentPassIndex].resize(variantsNumber);
  if (_pipelineType == AbstractPipeline::COMPUTE_PIPELINE)
  {
    Abort("Not implemented");
  }
  else
  {
    context.currentPass->graphicPipelineVariants.resize(variantsNumber);
  }
}

// Это рекурсивная функция, пребирающая все варианты селекшенов
void PassConfigurator::_processShadersSeveralVariants(
                                      uint32_t selectionIndex,
                                      ConfigurationBuildContext& context) const
{
  const std::string& selectionName = _selections[selectionIndex];
  TechniqueConfiguration::SelectionDefine* selection =
                                          findSelection(context, selectionName);
  MT_ASSERT(selection != nullptr);
  for(const std::string& selectionValue : selection->valueVariants)
  {
    context.currentDefines[selectionIndex].name = selectionName.c_str();
    context.currentDefines[selectionIndex].value = selectionValue.c_str();

    if(selectionIndex == 0)
    {
      //  Мы обрабатываем последний селекшен из списка, пора переходить к
      //  шейдерам
      _processShadersOneVariant(context);
      context.currentVariantIndex++;
    }
    else
    {
      //  Это ещё не последний селекшен, переходим к следующему из списка
      _processShadersSeveralVariants(selectionIndex - 1, context);
    }
  }
}

void PassConfigurator::_processShadersOneVariant(
                                      ConfigurationBuildContext& context) const
{
  for (const ShaderInfo& shader : _shaders)
  {
    _processShader(shader, context);
  }
}

void PassConfigurator::_processShader(const ShaderInfo& shaderRecord,
                                      ConfigurationBuildContext& context) const
{
  ShaderSet& shaderSet =
        context.shaders[context.currentPassIndex][context.currentVariantIndex];

  // Проверяем, что нет дублирования шейдерных стадий
  for(const ShaderModuleInfo& shader : shaderSet)
  {
    if(shader.stage == shaderRecord.stage)
    {
      throw std::runtime_error(_name + ": duplicate shader stages: " + shaderRecord.filename.c_str());
    }
  }

  // Получаем spirv код
  std::vector<uint32_t> spirData =
                      ShaderCompilator::compile(shaderRecord.filename.c_str(),
                                                shaderRecord.stage,
                                                context.currentDefines);

  // Парсим spirv код
  spv_reflect::ShaderModule reflection( spirData.size() * sizeof(spirData[0]),
                                        spirData.data());
  if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
  {
    throw std::runtime_error(_name + ": unable to process SPIRV data: " + shaderRecord.filename.c_str());
  }
  const SpvReflectShaderModule& reflectModule = reflection.GetShaderModule();

  // Ищем нужные нам данные в рефлексии
  _processShaderReflection(shaderRecord, reflectModule, context);

  // Создаем шейдерный модуль
  std::unique_ptr<ShaderModule> newShaderModule(
                                    new ShaderModule(
                                                *context.device,
                                                spirData,
                                                shaderRecord.filename.c_str()));
  shaderSet.push_back(ShaderModuleInfo{ std::move(newShaderModule),
                                        shaderRecord.stage});
}

void PassConfigurator::_processShaderReflection(
                                      const ShaderInfo& shaderRecord,
                                      const SpvReflectShaderModule& reflection,
                                      ConfigurationBuildContext& context) const
{
  for(uint32_t iSet = 0; iSet < reflection.descriptor_set_count; iSet++)
  {
    const SpvReflectDescriptorSet& reflectedSet =
                                              reflection.descriptor_sets[iSet];
    uint32_t setIndex = reflectedSet.set;
    if(setIndex > maxDescriptorSetIndex)
    {
      throw std::runtime_error(_name + ": descriptor set with the wrong set index: " + shaderRecord.filename);
    }

    DescriptorSetType setType = DescriptorSetType(setIndex);
    TechniqueConfiguration::DescriptorSet& set = _getOrCreateSet( setType,
                                                                  context);
    _processBindings(shaderRecord, set, reflectedSet, context);
  }
}

TechniqueConfiguration::DescriptorSet&
                  PassConfigurator::_getOrCreateSet(
                                      DescriptorSetType type,
                                      ConfigurationBuildContext& context) const
{
  for(TechniqueConfiguration::DescriptorSet& set :
                                          context.configuration->descriptorSets)
  {
    if(set.type == type) return set;
  }

  context.configuration->descriptorSets.push_back({.type = type});
  return context.configuration->descriptorSets.back();
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

void PassConfigurator::_processBindings(
                                    const ShaderInfo& shaderRecord,
                                    TechniqueConfiguration::DescriptorSet& set,
                                    const SpvReflectDescriptorSet& reflectedSet,
                                    ConfigurationBuildContext& context) const
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
      throw std::runtime_error(_name + ": " + shaderRecord.filename + ": only images arrays are supported.");
    }

    // Смотрим, возможно этот биндинг уже появлялся на других стадиях
    bool itIsNewResource = true;
    for(TechniqueConfiguration::Resource& resource :
                                              context.configuration->resources)
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
          Log::warning() << _name << ": binding conflict: " << shaderRecord.filename << " set: " << uint32_t(resource.set) << " binding: " << resource.bindingIndex;
          throw std::runtime_error(_name + ": binding conflict: " + shaderRecord.filename);
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
      context.configuration->resources.push_back(newResource);
    }

    // Если это юниформ буффер - парсим его
    if(reflectedBinding->descriptor_type ==
                                    SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
      _processUniformBlock(shaderRecord,*reflectedBinding, context);
    }
  }
}

void PassConfigurator::_processUniformBlock(
                          const ShaderInfo& shaderRecord,
                          const SpvReflectDescriptorBinding& reflectedBinding,
                          ConfigurationBuildContext& context) const
{
  const SpvReflectBlockVariable& block = reflectedBinding.block;

  // Для начала ищем, возможно буфер с таким биндингом уже встречался
  for(TechniqueConfiguration::UniformBuffer& buffer :
                                          context.configuration->uniformBuffers)
  {
    if( uint32_t(buffer.set) == reflectedBinding.set &&
        buffer.binding == reflectedBinding.binding)
    {
      // Нашли буфер с таким же биндингом, проверяем имя и размер
      if( buffer.name != reflectedBinding.name ||
          buffer.size != block.size)
      {
        Log::warning() << _name << ": uniform buffer mismatch set:" << reflectedBinding.set << " binding: " << reflectedBinding.binding << " file:" << shaderRecord.filename;
        throw std::runtime_error(_name + ": uniform buffer mismatch: " + shaderRecord.filename);
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
  if(newBuffer.set == DescriptorSetType::VOLATILE)
  {
    newBuffer.volatileContextOffset =
                              context.configuration->volatileUniformBuffersSize;
    context.configuration->volatileUniformBuffersSize += newBuffer.size;
  }
  else
  {
    newBuffer.volatileContextOffset = 0;
  }
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

  context.configuration->uniformBuffers.push_back(newBuffer);
}

void PassConfigurator::_parseUniformBlockMember(
                                const ShaderInfo& shaderRecord,
                                TechniqueConfiguration::UniformBuffer& target,
                                const SpvReflectBlockVariable& sourceMember,
                                std::string namePrefix,
                                uint32_t parentBlockOffset) const
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

void PassConfigurator::createPipelines(ConfigurationBuildContext& context) const
{
  MT_ASSERT(context.configuration->pipelineLayout != nullptr)

  for(uint32_t variant = 0;
      variant < context.currentPass->graphicPipelineVariants.size();
      variant++)
  {
    //  Формируем список шейдерных модулей, из которых состоит пайплайн
    ShaderSet& shaderSet = context.shaders[context.currentPassIndex][variant];
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
      context.currentPass->graphicPipelineVariants[variant] =
                    ConstRef(new GraphicPipeline(
                                      *_frameBufferFormat,
                                      shaders,
                                      _topology,
                                      _rasterizationState,
                                      _depthStencilState,
                                      _blendingState,
                                      *context.configuration->pipelineLayout));
    }
  }
}
