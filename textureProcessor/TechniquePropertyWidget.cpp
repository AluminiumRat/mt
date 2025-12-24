#include <imgui.h>

#include <yaml-cpp/yaml.h>

#include <gui/GUIWindow.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/modalDialogs.h>
#include <technique/TechniqueLoader.h>
#include <resourceManagement/BufferResourceManager.h>
#include <resourceManagement/TextureManager.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>
#include <util/vkMeta.h>

#include <TechniquePropertyWidget.h>

namespace fs = std::filesystem;

TechniquePropertyWidget::TechniquePropertyWidget(
                                mt::Technique& technique,
                                const std::string& fullName,
                                const std::string& shortName,
                                const TechniquePropsWidgetCommon& commonData) :
  _technique(technique),
  _fullName(fullName),
  _shortName(shortName),
  _commonData(commonData),
  _active(false),
  _unsupportedType(false),
  _uniform(nullptr),
  _vectorSize(1),
  _intValue(1),
  _floatValue(1),
  _resourceBinding(nullptr),
  _resourceType(VK_DESCRIPTOR_TYPE_SAMPLER)
{
  updateFromTechnique();
}

void TechniquePropertyWidget::updateFromTechnique()
{
  _active = false;
  _unsupportedType = false;
  _uniform = nullptr;
  _resourceBinding = nullptr;

  const mt::TechniqueConfiguration::UniformVariable* uniformDescription =
                                                      _getUniformDescription();
  if(uniformDescription != nullptr)
  {
    _scalarType = uniformDescription->baseType;
    _vectorSize = uniformDescription->vectorSize;
    if(_vectorSize == 0) _vectorSize = 1;
    if( _scalarType == mt::TechniqueConfiguration::UNKNOWN_TYPE ||
        uniformDescription->isMatrix ||
        uniformDescription->isArray ||
        _vectorSize > 4)
    {
      _unsupportedType = true;
    }
    _uniform = &_technique.getOrCreateUniform(_fullName.c_str());
    _active = true;
    _updateUniformValue();
    return;
  }

  const mt::TechniqueConfiguration::Resource* resourceDescription =
                                                      _getResourceDescription();
  if(resourceDescription != nullptr)
  {
    _resourceType = resourceDescription->type;
    if( _resourceType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE &&
        _resourceType != VK_DESCRIPTOR_TYPE_SAMPLER &&
        _resourceType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
      _unsupportedType = true;
    }
    if( resourceDescription->writeAccess ||
        resourceDescription->count > 1)
    {
      _unsupportedType = true;
    }

    _resourceBinding =
                      &_technique.getOrCreateResourceBinding(_fullName.c_str());
    _active = true;
    _updateResource();
    return;
  }
}

const mt::TechniqueConfiguration::UniformVariable*
                TechniquePropertyWidget::_getUniformDescription() const noexcept
{
  const mt::TechniqueConfiguration* configuration = _technique.configuration();
  if(configuration == nullptr) return nullptr;

  for(const mt::TechniqueConfiguration::UniformBuffer& buffer :
                                                  configuration->uniformBuffers)
  {
    if(buffer.set == mt::DescriptorSetType::COMMON) continue;
    for(const mt::TechniqueConfiguration::UniformVariable& variable :
                                                              buffer.variables)
    {
      if(variable.fullName == _fullName) return &variable;
    }
  }

  return nullptr;
}

const mt::TechniqueConfiguration::Resource*
              TechniquePropertyWidget::_getResourceDescription() const noexcept
{
  const mt::TechniqueConfiguration* configuration = _technique.configuration();
  if (configuration == nullptr) return nullptr;

  for (const mt::TechniqueConfiguration::Resource& resource :
                                                      configuration->resources)
  {
    if(resource.set == mt::DescriptorSetType::COMMON) continue;
    if(resource.name == _fullName) return &resource;
  }

  return nullptr;
}

void TechniquePropertyWidget::_updateUniformValue()
{
  if(_uniform == nullptr) return;

  if (_scalarType == mt::TechniqueConfiguration::INT_TYPE)
  {
    mt::UniformVariable::ValueRef valueRef;
    valueRef.data = &_intValue;
    valueRef.dataSize = _vectorSize * sizeof(_intValue[0]);
    _uniform->setValue(valueRef);
  }
  else if (_scalarType == mt::TechniqueConfiguration::FLOAT_TYPE)
  {
    mt::UniformVariable::ValueRef valueRef;
    valueRef.data = &_floatValue;
    valueRef.dataSize = _vectorSize * sizeof(_floatValue[0]);
    _uniform->setValue(valueRef);
  }
}

void TechniquePropertyWidget::_updateResource()
{
  if(_resourceBinding == nullptr) return;

  _resourceBinding->clear();

  if(_resourceType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    if(_resourcePath.empty()) return;
    mt::ConstRef<mt::TechniqueResource> resource =
            _commonData.textureManager->scheduleLoading(
                                                _resourcePath,
                                                *_commonData.resourceOwnerQueue,
                                                false);
    _resourceBinding->setResource(resource);
  }
  else if(_resourceType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if(_resourcePath.empty()) return;
    mt::ConstRef<mt::TechniqueResource> resource =
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
    mt::Log::error() << "TechniquePropertyWidget::unsupported resource type";
  }
}

void TechniquePropertyWidget::_updateSampler()
{
  SamplerValue& samplerValue = getSamplerValue();

  mt::Device& device = _technique.device();

  if(samplerValue.mode == CUSTOM_SAMPLER_MODE)
  {
    // Кастомный сэмплер
    mt::Ref<mt::Sampler> newSampler(
                            new mt::Sampler(device, samplerValue.description));
    _resourceBinding->setSampler(newSampler);
  }
  else
  {
    // Дефолтный сэмплер
    // Сначала ищем дефолтный сэмплер в конфигурации
    const mt::TechniqueConfiguration* configuration =
                                                    _technique.configuration();
    if(configuration != nullptr)
    {
      for(const mt::TechniqueConfiguration::DefaultSampler& sampler :
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
    mt::Ref<mt::Sampler> newSampler(new mt::Sampler(device));
    _resourceBinding->setSampler(newSampler);
  }
}

void TechniquePropertyWidget::makeGUI()
{
  if(_unsupportedType)
  {
    ImGui::Text("Unsupported type");
    return;
  }

  if(_uniform != nullptr)
  {
    _makeUniformGUI();
    return;
  }

  if(_resourceBinding != nullptr)
  {
    _makeResourceGUI();
    return;
  }

  ImGui::Text("Unsupported type");
}

void TechniquePropertyWidget::_makeUniformGUI()
{
  if(_scalarType == mt::TechniqueConfiguration::INT_TYPE)
  {
    int newValue[] = {_intValue[0], _intValue[1], _intValue[2], _intValue[3]};
    switch(_vectorSize)
    {
      case 1:
        ImGui::InputInt(_fullName.c_str(), newValue, 0);
        break;
      case 2:
        ImGui::InputInt2(_fullName.c_str(), newValue, 0);
        break;
      case 3:
        ImGui::InputInt3(_fullName.c_str(), newValue, 0);
        break;
      case 4:
        ImGui::InputInt4(_fullName.c_str(), newValue, 0);
        break;
    }
    glm::ivec4 newValueVec(newValue[0], newValue[1], newValue[2], newValue[3]);
    if(newValueVec != _intValue)
    {
      _intValue = newValueVec;
      _updateUniformValue();
    }
  }
  else if(_scalarType == mt::TechniqueConfiguration::FLOAT_TYPE)
  {
    glm::vec4 newValue = _floatValue;
    switch(_vectorSize)
    {
      case 1:
        ImGui::InputFloat(_fullName.c_str(), (float*)&newValue, 0);
        break;
      case 2:
        ImGui::InputFloat2(_fullName.c_str(), (float*)&newValue, 0);
        break;
      case 3:
        ImGui::InputFloat3(_fullName.c_str(), (float*)&newValue, 0);
        break;
      case 4:
        ImGui::InputFloat4(_fullName.c_str(), (float*)&newValue, 0);
        break;
    }
    if(newValue != _floatValue)
    {
      _floatValue = newValue;
      _updateUniformValue();
    }
  }
  else ImGui::Text("Unsupported type");
}

void TechniquePropertyWidget::_makeResourceGUI()
{
  switch(_resourceType)
  {
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    _makeTextureGUI();
    break;
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    _makeBufferGUI();
    break;
  case VK_DESCRIPTOR_TYPE_SAMPLER:
    _makeSamplerGUI();
    break;
  default:
    ImGui::Text("Unsupported type");
  }
}

void TechniquePropertyWidget::_makeTextureGUI()
{
  if(mt::fileSelectionLine(_fullName.c_str(), _resourcePath))
  {
    try
    {
      fs::path file =
              mt::openFileDialog(
                          mt::GUIWindow::currentWindow(),
                          mt::FileFilters{{ .expression = "*.dds",
                                            .description = "DDS image(*.dds)"}},
                          "");
      if(!file.empty())
      {
        _resourcePath = file;
        _updateResource();
      }
    }
    catch (std::exception& error)
    {
      mt::Log::error() << error.what();
      mt::errorDialog(mt::GUIWindow::currentWindow(), "Error", "Unable to open texture file");
    }
  }
}

void TechniquePropertyWidget::_makeBufferGUI()
{
  if(mt::fileSelectionLine(_fullName.c_str(), _resourcePath))
  {
    try
    {
      fs::path file =
              mt::openFileDialog(
                      mt::GUIWindow::currentWindow(),
                      mt::FileFilters{{ .expression = "*.bin",
                                        .description = "Binary files(*.bin)"}},
                      "");
      if(!file.empty())
      {
        _resourcePath = file;
        _updateResource();
      }
    }
    catch (std::exception& error)
    {
      mt::Log::error() << error.what();
      mt::errorDialog(mt::GUIWindow::currentWindow(), "Error", "Unable to open file");
    }
  }
}

TechniquePropertyWidget::SamplerValue&
                                      TechniquePropertyWidget::getSamplerValue()
{
  if(_samplerValue != nullptr) return *_samplerValue;
  _samplerValue.reset(new SamplerValue);
  return *_samplerValue;
}

void TechniquePropertyWidget::_makeSamplerGUI()
{
  _samplerModeGUI();

  SamplerValue& sampler = getSamplerValue();
  if(sampler.mode == CUSTOM_SAMPLER_MODE) _customSamplerGUI();
}

void TechniquePropertyWidget::_samplerModeGUI()
{
  SamplerValue& sampler = getSamplerValue();

  static const mt::Bimap<SamplerMode> modeMap{
    "Sampler mode",
    {
      {DEFAULT_SAMPLER_MODE, "Default"},
      {CUSTOM_SAMPLER_MODE, "Custom"}
    }};
  if(mt::enumSelectionCombo("##samplerMode", sampler.mode, modeMap))
  {
    _updateResource();
  }
}

void TechniquePropertyWidget::_customSamplerGUI()
{
  SamplerValue& sampler = getSamplerValue();
  mt::SamplerDescription& description = sampler.description;

  bool update = false;

  mt::ImGuiPropertyGrid samplerGrid("##samplerProps");
  samplerGrid.addRow("Min filter:");
  update |= mt::enumSelectionCombo( "##Minfilter",
                                    description.minFilter,
                                    mt::filterMap);

  samplerGrid.addRow("Mag filter:");
  update |= mt::enumSelectionCombo( "##Magfilter",
                                    description.magFilter,
                                    mt::filterMap);

  samplerGrid.addRow("Mipmap mode:");
  update |= mt::enumSelectionCombo( "##MipmapMode",
                                    description.mipmapMode,
                                    mt::mipmapModeMap);

  samplerGrid.addRow("Min LOD:");
  update |= ImGui::InputFloat("##MinLod", &description.minLod);

  samplerGrid.addRow("Max LOD:");
  update |= ImGui::InputFloat("##MaxLod", &description.maxLod);

  samplerGrid.addRow("LOD bias:");
  update |= ImGui::InputFloat("##LODBias", &description.mipLodBias);

  samplerGrid.addRow("Anisotropy:");
  update |= ImGui::Checkbox("##AnisotropyEnable",
    &description.anisotropyEnable);

  samplerGrid.addRow("Max anisotropy:");
  update |= ImGui::InputFloat("##MaxAnisotropy", &description.maxAnisotropy);

  samplerGrid.addRow("Address U:");
  update |= mt::enumSelectionCombo( "##AddressModeU",
                                    description.addressModeU,
                                    mt::addressModeMap);

  samplerGrid.addRow("Address V:");
  update |= mt::enumSelectionCombo( "##AddressModeV",
                                    description.addressModeV,
                                    mt::addressModeMap);

  samplerGrid.addRow("Address W:");
  update |= mt::enumSelectionCombo( "##AddressModeW",
                                    description.addressModeW,
                                    mt::addressModeMap);

  samplerGrid.addRow("Compare enable:");
  update |= ImGui::Checkbox("##CompareEnable", &description.compareEnable);

  samplerGrid.addRow("Compare op:");
  update |= mt::enumSelectionCombo( "##CompareOp",
                                    description.compareOp,
                                    mt::compareOpMap);

  samplerGrid.addRow("Border:");
  update |= mt::enumSelectionCombo( "##Border",
                                    description.borderColor,
                                    mt::borderColorMap);

  samplerGrid.addRow("UCoord:", "Unnormalized coordinates");
  update |= ImGui::Checkbox("##UCoord", &description.unnormalizedCoordinates);

  if(update) _updateResource();
}

void TechniquePropertyWidget::save(YAML::Emitter& target) const
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
    _saveResource(target);
  }
  else
  {
    target << YAML::Value << "unknown";
  }

  target << YAML::EndMap;
}

void TechniquePropertyWidget::_saveUniform(YAML::Emitter& target) const
{
  if(_scalarType == mt::TechniqueConfiguration::FLOAT_TYPE)
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

void TechniquePropertyWidget::_saveResource(YAML::Emitter& target) const
{
  fs::path projectFolder = _commonData.projectFile->parent_path();
  target << YAML::Key << "file";
  target << YAML::Value << mt::pathToUtf8(mt::makeStoredPath( _resourcePath,
                                                              projectFolder));
}

void TechniquePropertyWidget::_saveSampler(YAML::Emitter& target) const
{
  if(_samplerValue == nullptr) return;

  target << YAML::Key << "default";
  target << YAML::Value << (_samplerValue->mode == DEFAULT_SAMPLER_MODE);

  if(_samplerValue->mode == DEFAULT_SAMPLER_MODE) return;

  const mt::SamplerDescription& description = _samplerValue->description;

  target << YAML::Key << "magFilter";
  target << YAML::Value << mt::filterMap[description.magFilter];

  target << YAML::Key << "minFilter";
  target << YAML::Value << mt::filterMap[description.minFilter];

  target << YAML::Key << "mipmapMode";
  target << YAML::Value << mt::mipmapModeMap[description.mipmapMode];

  target << YAML::Key << "addressModeU";
  target << YAML::Value << mt::addressModeMap[description.addressModeU];

  target << YAML::Key << "addressModeV";
  target << YAML::Value << mt::addressModeMap[description.addressModeV];

  target << YAML::Key << "addressModeW";
  target << YAML::Value << mt::addressModeMap[description.addressModeW];

  target << YAML::Key << "mipLodBias";
  target << YAML::Value << description.mipLodBias;

  target << YAML::Key << "anisotropyEnable";
  target << YAML::Value << description.anisotropyEnable;

  target << YAML::Key << "maxAnisotropy";
  target << YAML::Value << description.maxAnisotropy;

  target << YAML::Key << "compareEnable";
  target << YAML::Value << description.compareEnable;

  target << YAML::Key << "compareOp";
  target << YAML::Value << mt::compareOpMap[description.compareOp];

  target << YAML::Key << "minLod";
  target << YAML::Value << description.minLod;

  target << YAML::Key << "maxLod";
  target << YAML::Value << description.maxLod;

  target << YAML::Key << "borderColor";
  target << YAML::Value << mt::borderColorMap[description.borderColor];

  target << YAML::Key << "unnormalizedCoordinates";
  target << YAML::Value << description.unnormalizedCoordinates;
}

std::string TechniquePropertyWidget::readFullName(const YAML::Node& source)
{
  return source["fullName"].as<std::string>("");
}

std::string TechniquePropertyWidget::readShortName(const YAML::Node& source)
{
  return source["shortName"].as<std::string>("");
}

void TechniquePropertyWidget::load(const YAML::Node& source)
{
  std::string type = source["type"].as<std::string>("no type");
  if(type == "uniform")
  {
    _readUniform(source);
    _updateUniformValue();
  }
  else if (type == "resource")
  {
    _readResource(source);
    _updateResource();
  }
  else if (type == "sampler")
  {
    _readSampler(source);
    _updateResource();
  }
  else throw std::runtime_error(_fullName + ": unknown property type: " + type);
}

void TechniquePropertyWidget::_readUniform(const YAML::Node& source)
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

void TechniquePropertyWidget::_readResource(const YAML::Node& source)
{
  YAML::Node fileNode = source["file"];
  if(!fileNode.IsDefined()) return;

  std::string filename = fileNode.as<std::string>("");

  _resourcePath = mt::utf8ToPath(filename);
  fs::path projectFolder = _commonData.projectFile->parent_path();
  _resourcePath = mt::restoreAbsolutePath(_resourcePath, projectFolder);
}

void TechniquePropertyWidget::_readSampler(const YAML::Node& source)
{
  SamplerValue& samplerValue = getSamplerValue();

  bool isDefault = source["default"].as<bool>(true);
  if(isDefault)
  {
    samplerValue.mode = DEFAULT_SAMPLER_MODE;
    return;
  }

  samplerValue.mode = CUSTOM_SAMPLER_MODE;
  samplerValue.description = mt::loadSamplerDescription(source);
}
