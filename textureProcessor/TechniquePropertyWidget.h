#pragma once

#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <util/Ref.h>
#include <technique/ResourceBinding.h>
#include <technique/Technique.h>
#include <technique/TechniqueResource.h>
#include <technique/UniformVariable.h>

class TechniquePropertyWidget
{
public:
  TechniquePropertyWidget(mt::Technique& technique,
                          const std::string& fullName,
                          const std::string& shortName);
  TechniquePropertyWidget(const TechniquePropertyWidget&) = delete;
  TechniquePropertyWidget& operator = (const TechniquePropertyWidget&) = delete;
  ~TechniquePropertyWidget() noexcept = default;

  inline bool active() const noexcept;

  inline const std::string& fullName() const noexcept;
  inline const std::string& shortName() const noexcept;

  void makeGUI();
  void updateFromTechnique();

private:
  const mt::TechniqueConfiguration::UniformVariable*
                                      _getUniformDescription() const noexcept;
  const mt::TechniqueConfiguration::Resource*
                                      _getResourceDescription() const noexcept;
  void _updateUniformValue();
  void _updateResource();
  void _makeUniformGUI();

private:
  mt::Technique& _technique;
  std::string _fullName;
  std::string _shortName;
  bool _active;
  bool _unsupportedType;

  mt::UniformVariable* _uniform;
  mt::TechniqueConfiguration::BaseScalarType _scalarType;
  uint32_t _vectorSize;
  glm::ivec4 _intValue;
  glm::vec4 _floatValue;

  mt::ResourceBinding* _resourceBinding;
  VkDescriptorType _resourceType;
  std::filesystem::path _resourcePath;
};

inline bool TechniquePropertyWidget::active() const noexcept
{
  return _active;
}

inline const std::string& TechniquePropertyWidget::fullName() const noexcept
{
  return _fullName;
}

inline const std::string& TechniquePropertyWidget::shortName() const noexcept
{
  return _shortName;
}
