#include <stdexcept>

#include <spirv_reflect.h>

#include <technique/DescriptorSetType.h>
#include <technique/Technique.h>
#include <util/Abort.h>
#include <util/Log.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/pipeline/ShaderModule.h>

using namespace mt;

Technique::Technique(Device& device) noexcept :
  _device(device),
  _revision(0),
  _needRebuildConfiguration(true),
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

void Technique::_invalidateConfiguration() noexcept
{
  _configuration.reset();
  _needRebuildConfiguration = true;
  _revision++;
}

void Technique::_buildConfiguration()
{
  try
  {
    _needRebuildConfiguration = false;

    _configuration = Ref(new TechniqueConfiguration);

    _configuration->pipelineType = _pipelineType;
    if( _pipelineType == AbstractPipeline::GRAPHIC_PIPELINE &&
        !_frameBufferFormat.has_value())
    {
      throw std::runtime_error("Technique: pipeline type is GRAPHIC_PIPELINE but the frame buffer format is empty");
    }

    // Загружаем шейдеры
    if(_shaders.empty())
    {
      Log::warning() << "Technique: attempt to create a configuration without shaders";
      return;
    }
    ConfigurationBuildContext buildContext;
    for(const ShaderInfo& shader : _shaders)
    {
      _processShader(shader, buildContext);
    }

    _createLayouts(buildContext);
    _createPipeline(buildContext);
  }
  catch(...)
  {
    _configuration.reset();
    throw;
  }
}

void Technique::_processShader( const ShaderInfo& shaderRecord,
                                ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);

  // Проверяем, что нет дублирования шейдерных стадий
  for(const ShaderModuleInfo& shader : buildContext.shaders)
  {
    if(shader.stage == shaderRecord.stage)
    {
      throw std::runtime_error(std::string("Duplicate shader stages: ") + shaderRecord.filename.c_str());
    }
  }

  // Получаем spirv код
  std::vector<uint32_t> spirData =
      ShaderModule::getShaderLoader().loadShader(shaderRecord.filename.c_str());

  // Парсим spirv код
  spv_reflect::ShaderModule reflection( spirData.size() * sizeof(spirData[0]),
                                        spirData.data());
  if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
  {
    throw std::runtime_error(std::string("Unable to process SPIRV data: ") + shaderRecord.filename.c_str());
  }
  const SpvReflectShaderModule& reflectModule = reflection.GetShaderModule();

  // Ищем нужные нам данные в рефлексии
  _processDescriptorSets(shaderRecord, reflectModule);

  // Создаем шейдерный модуль
  std::unique_ptr<ShaderModule> newShaderModule(
                                    new ShaderModule(
                                                _device,
                                                spirData,
                                                shaderRecord.filename.c_str()));
  buildContext.shaders.push_back(ShaderModuleInfo{std::move(newShaderModule),
                                                  shaderRecord.stage});
}

void Technique::_processDescriptorSets(
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
      throw std::runtime_error(std::string("Technique: Descriptor set with the wrong set index: ") + shaderRecord.filename);
    }

    DescriptorSetType setType = DescriptorSetType(setIndex);
    TechniqueConfiguration::DescriptorSet& set = _getOrCreateSet(setType);
    _processBindings(shaderRecord, set, reflectedSet);
  }
}

TechniqueConfiguration::DescriptorSet& Technique::_getOrCreateSet(
                                                        DescriptorSetType type)
{
  for(TechniqueConfiguration::DescriptorSet& set :
                                                _configuration->descriptorSets)
  {
    if(set.type == type) return set;
  }

  _configuration->descriptorSets.push_back({.type = type});
  return _configuration->descriptorSets.back();
}

void Technique::_processBindings( const ShaderInfo& shaderRecord,
                                  TechniqueConfiguration::DescriptorSet& set,
                                  const SpvReflectDescriptorSet& reflectedSet)
{
  for(uint32_t iBinding = 0; iBinding < reflectedSet.binding_count; iBinding++)
  {
    const SpvReflectDescriptorBinding* reflectedBinding =
                                                reflectedSet.bindings[iBinding];
    MT_ASSERT(reflectedBinding != nullptr);

    // Для начала просто собираем данные из рефлексии
    TechniqueConfiguration::Resource newResource;
    newResource.type = VkDescriptorType(reflectedBinding->descriptor_type);
    newResource.set = set.type;
    newResource.bindingIndex = reflectedBinding->binding;
    newResource.stages = shaderRecord.stage;
    newResource.name = reflectedBinding->name;
    newResource.writeAccess =
              reflectedBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_UAV;
    newResource.count = reflectedBinding->count;

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
          Log::warning() << "Technique: binding conflict: " << shaderRecord.filename << " set: " << uint32_t(resource.set) << " binding: " << resource.bindingIndex;
          throw std::runtime_error(std::string("Technique: binding conflict: ") + shaderRecord.filename);
        }
        // Биндинги совместимы, просто дополняем информацию
        resource.stages |= newResource.stages;
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

void Technique::_processUniformBlock(
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
        Log::warning() << "Technique: uniform buffer mismatch set:" << reflectedBinding.set << " binding: " << reflectedBinding.binding << " file:" << shaderRecord.filename;
        throw std::runtime_error(std::string("Technique: Uniform buffer mismatch: ") + shaderRecord.filename);
      }
      // Похоже, что это один и тот же буфер, обновим для него информацию
      buffer.stages |= shaderRecord.stage;
      return;
    }
  }

  // Этот буфер раньше не встречался, посмотрим что внутри
  TechniqueConfiguration::UniformBuffer newBuffer{};
  newBuffer.set = DescriptorSetType(reflectedBinding.set);
  newBuffer.binding = reflectedBinding.binding;
  newBuffer.name = reflectedBinding.name;
  newBuffer.size = block.size;
  newBuffer.stages = shaderRecord.stage;
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

void Technique::_parseUniformBlockMember(
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

void Technique::_createLayouts(ConfigurationBuildContext& buildContext)
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
    binding.descriptorCount = 1;
    binding.stageFlags = resource.stages;

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
  buildContext.pipelineLayout = ConstRef(new PipelineLayout(_device,
                                                            setLayouts));
}

void Technique::_createPipeline(ConfigurationBuildContext& buildContext)
{
  MT_ASSERT(_configuration != nullptr);
  MT_ASSERT(buildContext.pipelineLayout != nullptr)

  if(_pipelineType == AbstractPipeline::COMPUTE_PIPELINE)
  {
    Abort("Not implemented");
  }
  else
  {
    MT_ASSERT(_frameBufferFormat.has_value());

    std::vector<AbstractPipeline::ShaderInfo> shaders;
    for(ShaderModuleInfo& shader : buildContext.shaders)
    {
      shaders.push_back(AbstractPipeline::ShaderInfo{
                                          .module = shader.shaderModule.get(),
                                          .stage = shader.stage,
                                          .entryPoint = "main"});
    }
    _configuration->graphicPipeline =
                              ConstRef(new GraphicPipeline(
                                                *_frameBufferFormat,
                                                shaders,
                                                _topology,
                                                _rasterizationState,
                                                _depthStencilState,
                                                _blendingState,
                                                *buildContext.pipelineLayout));
  }
}
