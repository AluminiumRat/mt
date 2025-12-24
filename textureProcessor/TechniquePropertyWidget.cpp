#include <imgui.h>

#include <gui/GUIWindow.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/modalDialogs.h>
#include <resourceManagement/BufferResourceManager.h>
#include <resourceManagement/TextureManager.h>
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
  MT_ASSERT(_uniform != nullptr);

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
  MT_ASSERT(_resourceBinding != nullptr);

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
