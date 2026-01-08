#include <yaml-cpp/yaml.h>

#include <technique/TechniqueLoader.h>
#include <techniquePropertyGrid/TechniqueProperty.h>
#include <resourceManagement/BufferResourceManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <util/vkMeta.h>

namespace fs = std::filesystem;

using namespace mt;

TechniqueProperty::TechniqueProperty(
                                Technique& technique,
                                const std::string& fullName,
                                const std::string& shortName,
                                const TechniquePropertySetCommon& commonData) :
  _technique(technique),
  _fullName(fullName),
  _shortName(shortName),
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
    if(_vectorSize == 0) _vectorSize = 1;
    if( _scalarType == TechniqueConfiguration::UNKNOWN_TYPE ||
        uniformDescription->isMatrix ||
        uniformDescription->isArray ||
        _vectorSize > 4)
    {
      _unsupportedType = true;
    }

    _uniform = &_technique.getOrCreateUniform(_fullName.c_str());
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

    _resourceBinding =
                      &_technique.getOrCreateResourceBinding(_fullName.c_str());
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
      if(variable.fullName == _fullName) return &variable;
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
    if(resource.name == _fullName) return &resource;
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
        if(sampler.resourceName == _fullName)
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
  target << YAML::BeginMap;

  target << YAML::Key << "fullName";
  target << YAML::Value << _fullName;

  target << YAML::Key << "shortName";
  target << YAML::Value << _shortName;

  target << YAML::Key << "type";
  if(_uniform != nullptr)
  {
    target << YAML::Value << "uniform";
    _saveUniform(target);
  }
  else if ( _resourceBinding != nullptr &&
            _resourceType == VK_DESCRIPTOR_TYPE_SAMPLER)
  {
    target << YAML::Value << "sampler";
    _saveSampler(target);
  }
  else if(_resourceBinding != nullptr)
  {
    target << YAML::Value << "resource";
    _saveResource(target, resourcesRootFolder);
  }
  else
  {
    target << YAML::Value << "unknown";
  }

  target << YAML::EndMap;
}

void TechniqueProperty::_saveUniform(YAML::Emitter& target) const
{
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
}

void TechniqueProperty::_saveResource(
                        YAML::Emitter& target,
                        const fs::path& resourcesRootFolder) const
{
  target << YAML::Key << "file";
  target << YAML::Value << pathToUtf8(makeStoredPath( _resourcePath,
                                                      resourcesRootFolder));
}

void TechniqueProperty::_saveSampler(YAML::Emitter& target) const
{
  if(_samplerValue == nullptr) return;

  target << YAML::Key << "default";
  target << YAML::Value << (_samplerValue->mode == DEFAULT_SAMPLER_MODE);

  if(_samplerValue->mode == DEFAULT_SAMPLER_MODE) return;

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

std::string TechniqueProperty::readFullName(const YAML::Node& source)
{
  return source["fullName"].as<std::string>("");
}

std::string TechniqueProperty::readShortName(const YAML::Node& source)
{
  return source["shortName"].as<std::string>("");
}

void TechniqueProperty::load( const YAML::Node& source,
                              const fs::path& resourcesRootFolder)
{
  std::string type = source["type"].as<std::string>("no type");
  if(type == "uniform")
  {
    _readUniform(source);
    _updateUniformValue();
  }
  else if (type == "resource")
  {
    _readResource(source, resourcesRootFolder);
    _updateResource();
  }
  else if (type == "sampler")
  {
    _readSampler(source);
    _updateResource();
  }
  else throw std::runtime_error(_fullName + ": unknown property type: " + type);
}

void TechniqueProperty::_readUniform(const YAML::Node& source)
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
  }
}

void TechniqueProperty::_readResource(const YAML::Node& source,
                                      const fs::path& resourcesRootFolder)
{
  YAML::Node fileNode = source["file"];
  if(!fileNode.IsDefined()) return;

  std::string filename = fileNode.as<std::string>("");

  _resourcePath = utf8ToPath(filename);
  _resourcePath = restoreAbsolutePath(_resourcePath, resourcesRootFolder);
}

void TechniqueProperty::_readSampler(const YAML::Node& source)
{
  SamplerValue& samplerValue = _getOrCreateSamplerValue();

  bool isDefault = source["default"].as<bool>(true);
  if(isDefault)
  {
    samplerValue.mode = DEFAULT_SAMPLER_MODE;
    return;
  }

  samplerValue.mode = CUSTOM_SAMPLER_MODE;
  samplerValue.description = loadSamplerDescription(source);
}
