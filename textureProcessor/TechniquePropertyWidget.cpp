#include <TechniquePropertyWidget.h>

#include <imgui.h>

TechniquePropertyWidget::TechniquePropertyWidget(
                                                mt::Technique& technique,
                                                const std::string& fullName,
                                                const std::string& shortName) :
  _technique(technique),
  _fullName(fullName),
  _shortName(shortName),
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
    ImGui::Text("Resource");
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
