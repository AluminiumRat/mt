#include <yaml-cpp/yaml.h>

#include <technique/TechniqueLoader.h>
#include <techniquePropertySet/TechniqueProperty.h>
#include <resourceManagement/BufferResourceManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <util/vkMeta.h>

namespace fs = std::filesystem;

using namespace mt;

TechniqueProperty::TechniqueProperty(
                                Technique& technique,
                                const std::string& name,
                                const TechniquePropertySetCommon& commonData) :
  _technique(technique),
  _name(name),
  _shortName(name),
  _commonData(commonData),
  _isActive(false),
  _unsupportedType(false),
  _guiHints(0),
  _uniform(nullptr),
  _vectorSize(1),
  _intValue(1),
  _floatValue(1),
  _resourceBinding(nullptr),
  _resourceType(VK_DESCRIPTOR_TYPE_SAMPLER)
{
  updateFromTechnique();
}

void TechniqueProperty::updateFromTechnique()
{
  _isActive = false;
  _unsupportedType = false;
  _guiHints = 0;
  _uniform = nullptr;
  _resourceBinding = nullptr;

  const TechniqueConfiguration::UniformVariable* uniformDescription =
                                                      _getUniformDescription();
  if(uniformDescription != nullptr)
  {
    _scalarType = uniformDescription->baseType;
    _vectorSize = uniformDescription->vectorSize;
    _guiHints = uniformDescription->guiHints;
    _shortName = uniformDescription->shortName;
    if(_vectorSize == 0) _vectorSize = 1;
    if( _scalarType == TechniqueConfiguration::UNKNOWN_TYPE ||
        uniformDescription->isMatrix ||
        uniformDescription->isArray ||
        _vectorSize > 4)
    {
      _unsupportedType = true;
    }

    _uniform = &_technique.getOrCreateUniform(_name.c_str());
    _updateUniformValue();
    _isActive = true;
    return;
  }

  const TechniqueConfiguration::Resource* resourceDescription =
                                                      _getResourceDescription();
  if(resourceDescription != nullptr)
  {
    _resourceType = resourceDescription->type;
    _guiHints = resourceDescription->guiHints;
    _shortName = _name;
    if( _resourceType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
        _resourceType != VK_DESCRIPTOR_TYPE_SAMPLER &&
        _resourceType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
      _unsupportedType = true;
    }
    if( resourceDescription->writeAccess || resourceDescription->count > 1)
    {
      _unsupportedType = true;
    }

    _resourceBinding = &_technique.getOrCreateResourceBinding(_name.c_str());
    _updateResource();
    _isActive = true;
    return;
  }
}

const TechniqueConfiguration::UniformVariable*
                      TechniqueProperty::_getUniformDescription() const noexcept
{
  const TechniqueConfiguration* configuration = _technique.configuration();
  if(configuration == nullptr) return nullptr;

  for(const TechniqueConfiguration::UniformBuffer& buffer :
                                                  configuration->uniformBuffers)
  {
    if(buffer.set == DescriptorSetType::COMMON) continue;
    for(const TechniqueConfiguration::UniformVariable& variable :
                                                              buffer.variables)
    {
      if(variable.fullName == _name) return &variable;
    }
  }

  return nullptr;
}

const TechniqueConfiguration::Resource*
                    TechniqueProperty::_getResourceDescription() const noexcept
{
  const TechniqueConfiguration* configuration = _technique.configuration();
  if (configuration == nullptr) return nullptr;

  for (const TechniqueConfiguration::Resource& resource :
                                                      configuration->resources)
  {
    if(resource.set == DescriptorSetType::COMMON) continue;
    if(resource.name == _name) return &resource;
  }

  return nullptr;
}

void TechniqueProperty::setUniformValue(const glm::ivec4& newValue)
{
  if(!isUniform()) return;
  if(_intValue == newValue) return;

  _intValue = newValue;
  _updateUniformValue();
}

void TechniqueProperty::setUniformValue(const glm::vec4& newValue)
{
  if(!isUniform()) return;
  if(_floatValue == newValue) return;

  _floatValue = newValue;
  _updateUniformValue();
}

void TechniqueProperty::setResourcePath(const std::filesystem::path& newValue)
{
  if (!isResource()) return;
  if (_resourcePath == newValue) return;
  _resourcePath = newValue;
  _updateResource();
}

void TechniqueProperty::setSamplerValue(const SamplerValue& newValue)
{
  if(!isSampler()) return;
  MT_ASSERT(_samplerValue != nullptr);
  if(*_samplerValue == newValue) return;
  *_samplerValue = newValue;
  _updateResource();
}

void TechniqueProperty::_updateUniformValue()
{
  if(_uniform == nullptr || _unsupportedType) return;

  if (_scalarType == TechniqueConfiguration::INT_TYPE)
  {
    UniformVariable::ValueRef valueRef;
    valueRef.data = &_intValue;
    valueRef.dataSize = _vectorSize * sizeof(_intValue[0]);
    _uniform->setValue(valueRef);
  }
  else if (_scalarType == TechniqueConfiguration::FLOAT_TYPE)
  {
    UniformVariable::ValueRef valueRef;
    valueRef.data = &_floatValue;
    valueRef.dataSize = _vectorSize * sizeof(_floatValue[0]);
    _uniform->setValue(valueRef);
  }
}

void TechniqueProperty::_updateResource()
{
  if(_resourceBinding == nullptr || _unsupportedType) return;

  _resourceBinding->clear();

  if(_resourceType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    if(_resourcePath.empty()) return;
    ConstRef<TechniqueResource> resource =
            _commonData.textureManager->scheduleLoading(
                                                _resourcePath,
                                                *_commonData.resourceOwnerQueue,
                                                false);
    _resourceBinding->setResource(resource);
  }
  else if(_resourceType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if(_resourcePath.empty()) return;
    ConstRef<TechniqueResource> resource =
            _commonData.bufferManager->scheduleLoading(
                                              _resourcePath,
                                              *_commonData.resourceOwnerQueue);
    _resourceBinding->setResource(resource);
  }
  else if(_resourceType == VK_DESCRIPTOR_TYPE_SAMPLER)
  {
    _updateSampler();
  }
  else
  {
    Log::error() << "TechniquePropertyWidget::unsupported resource type";
  }
}

void TechniqueProperty::_updateSampler()
{
  SamplerValue& samplerValue = _getOrCreateSamplerValue();

  Device& device = _technique.device();

  if(samplerValue.mode == CUSTOM_SAMPLER_MODE)
  {
    // Кастомный сэмплер
    Ref<Sampler> newSampler(new Sampler(device, samplerValue.description));
    _resourceBinding->setSampler(newSampler);
  }
  else
  {
    // Дефолтный сэмплер
    // Сначала ищем дефолтный сэмплер в конфигурации
    const TechniqueConfiguration* configuration = _technique.configuration();
    if(configuration != nullptr)
    {
      for(const TechniqueConfiguration::DefaultSampler& sampler :
                                                 configuration->defaultSamplers)
      {
        if(sampler.resourceName == _name)
        {
          _resourceBinding->setSampler(sampler.defaultSampler);
          return;
        }
      }
    }

    // Не нашли дефолтный сэмплер - создаем свой
    Ref<Sampler> newSampler(new Sampler(device));
    _resourceBinding->setSampler(newSampler);
  }
}

TechniqueProperty::SamplerValue& TechniqueProperty::_getOrCreateSamplerValue()
{
  if(_samplerValue != nullptr) return *_samplerValue;
  _samplerValue.reset(new SamplerValue);
  return *_samplerValue;
}

void TechniqueProperty::save( YAML::Emitter& target,
                              const fs::path& resourcesRootFolder) const
{
  if(_uniform != nullptr)
  {
    _saveUniform(target);
  }
  else if ( _resourceBinding != nullptr &&
            _resourceType == VK_DESCRIPTOR_TYPE_SAMPLER)
  {
    _saveSampler(target);
  }
  else if(_resourceBinding != nullptr)
  {
    _saveResource(target, resourcesRootFolder);
  }
}

void TechniqueProperty::_saveUniform(YAML::Emitter& target) const
{
  target << YAML::BeginMap;

  if(_scalarType == TechniqueConfiguration::FLOAT_TYPE)
  {
    target << YAML::Key << "float";
    std::vector<float> valueSequence( &_floatValue[0],
                                      &_floatValue[0] + _vectorSize);
    target << YAML::Value << YAML::Flow << valueSequence;
  }
  else
  {
    target << YAML::Key << "int";
    std::vector<int> valueSequence( &_intValue[0], &_intValue[0] + _vectorSize);
    target << YAML::Value << YAML::Flow << valueSequence;
  }

  target << YAML::EndMap;
}

void TechniqueProperty::_saveResource(
                        YAML::Emitter& target,
                        const fs::path& resourcesRootFolder) const
{
  target << YAML::BeginMap;

  target << YAML::Key << "file";
  target << YAML::Value << pathToUtf8(makeStoredPath( _resourcePath,
                                                      resourcesRootFolder));

  target << YAML::EndMap;
}

void TechniqueProperty::_saveSampler(YAML::Emitter& target) const
{
  if(_samplerValue == nullptr) return;

  target << YAML::BeginMap;
  target << YAML::Key << "sampler";

  target << YAML::BeginMap;

  target << YAML::Key << "default";
  target << YAML::Value << (_samplerValue->mode == DEFAULT_SAMPLER_MODE);

  if(_samplerValue->mode != DEFAULT_SAMPLER_MODE)
  {
    const SamplerDescription& description = _samplerValue->description;

    target << YAML::Key << "magFilter";
    target << YAML::Value << filterMap[description.magFilter];

    target << YAML::Key << "minFilter";
    target << YAML::Value << filterMap[description.minFilter];

    target << YAML::Key << "mipmapMode";
    target << YAML::Value << mipmapModeMap[description.mipmapMode];

    target << YAML::Key << "addressModeU";
    target << YAML::Value << addressModeMap[description.addressModeU];

    target << YAML::Key << "addressModeV";
    target << YAML::Value << addressModeMap[description.addressModeV];

    target << YAML::Key << "addressModeW";
    target << YAML::Value << addressModeMap[description.addressModeW];

    target << YAML::Key << "mipLodBias";
    target << YAML::Value << description.mipLodBias;

    target << YAML::Key << "anisotropyEnable";
    target << YAML::Value << description.anisotropyEnable;

    target << YAML::Key << "maxAnisotropy";
    target << YAML::Value << description.maxAnisotropy;

    target << YAML::Key << "compareEnable";
    target << YAML::Value << description.compareEnable;

    target << YAML::Key << "compareOp";
    target << YAML::Value << compareOpMap[description.compareOp];

    target << YAML::Key << "minLod";
    target << YAML::Value << description.minLod;

    target << YAML::Key << "maxLod";
    target << YAML::Value << description.maxLod;

    target << YAML::Key << "borderColor";
    target << YAML::Value << borderColorMap[description.borderColor];

    target << YAML::Key << "unnormalizedCoordinates";
    target << YAML::Value << description.unnormalizedCoordinates;
  }
  target << YAML::EndMap;
  target << YAML::EndMap;
}

void TechniqueProperty::load( const YAML::Node& source,
                              const fs::path& resourcesRootFolder)
{
  if(_tryReadUniform(source))
  {
    _updateUniformValue();
    return;
  }

 if (_tryReadResource(source, resourcesRootFolder))
  {
    _updateResource();
    return;
  }

  if (_tryReadSampler(source))
  {
    _updateResource();
    return;
  }

  throw std::runtime_error(_name + ": unknown property type");
}

bool TechniqueProperty::_tryReadUniform(const YAML::Node& source)
{
  YAML::Node floatNode = source["float"];
  if(floatNode.IsDefined() && floatNode.IsSequence())
  {
    int valueIndex = 0;
    for(YAML::Node valueNode : floatNode)
    {
      _floatValue[valueIndex] = valueNode.as<float>(1.0f);
      valueIndex++;
      if(valueIndex == 4) break;
    }
    return true;
  }

  YAML::Node intNode = source["int"];
  if(intNode.IsDefined() && intNode.IsSequence())
  {
    int valueIndex = 0;
    for(YAML::Node valueNode : intNode)
    {
      _intValue[valueIndex] = valueNode.as<int>(1);
      valueIndex++;
      if(valueIndex == 4) break;
    }
    return true;
  }
  return false;
}

bool TechniqueProperty::_tryReadResource( const YAML::Node& source,
                                          const fs::path& resourcesRootFolder)
{
  YAML::Node fileNode = source["file"];
  if(!fileNode.IsDefined()) return false;

  std::string filename = fileNode.as<std::string>("");

  _resourcePath = utf8ToPath(filename);
  _resourcePath = restoreAbsolutePath(_resourcePath, resourcesRootFolder);

  return true;
}

bool TechniqueProperty::_tryReadSampler(const YAML::Node& source)
{
  YAML::Node samplerNode = source["sampler"];
  if (!samplerNode.IsDefined()) return false;

  SamplerValue& samplerValue = _getOrCreateSamplerValue();

  bool isDefault = samplerNode["default"].as<bool>(true);
  if(isDefault)
  {
    samplerValue.mode = DEFAULT_SAMPLER_MODE;
    return true;
  }

  samplerValue.mode = CUSTOM_SAMPLER_MODE;
  samplerValue.description = loadSamplerDescription(samplerNode);

  return true;
}
