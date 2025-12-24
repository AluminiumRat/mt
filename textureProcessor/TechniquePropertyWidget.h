#pragma once

#include <memory>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <util/Ref.h>
#include <technique/ResourceBinding.h>
#include <technique/Technique.h>
#include <technique/TechniqueResource.h>
#include <technique/UniformVariable.h>
#include <vkr/Sampler.h>

#include <TechniquePropsWidgetCommon.h>

namespace YAML
{
  class Emitter;
  class Node;
};


class TechniquePropertyWidget
{
public:
  TechniquePropertyWidget(mt::Technique& technique,
                          const std::string& fullName,
                          const std::string& shortName,
                          const TechniquePropsWidgetCommon& commonData);
  TechniquePropertyWidget(const TechniquePropertyWidget&) = delete;
  TechniquePropertyWidget& operator = (const TechniquePropertyWidget&) = delete;
  ~TechniquePropertyWidget() noexcept = default;

  inline bool active() const noexcept;

  inline const std::string& fullName() const noexcept;
  inline const std::string& shortName() const noexcept;

  void makeGUI();
  void updateFromTechnique();

  void save(YAML::Emitter& target) const;

  static std::string readFullName(const YAML::Node& source);
  static std::string readShortName(const YAML::Node& source);
  void load(const YAML::Node& source);

private:
  enum SamplerMode
  {
    DEFAULT_SAMPLER_MODE,
    CUSTOM_SAMPLER_MODE
  };
  struct SamplerValue
  {
    SamplerMode mode = DEFAULT_SAMPLER_MODE;
    mt::SamplerDescription description;
  };

private:
  const mt::TechniqueConfiguration::UniformVariable*
                                      _getUniformDescription() const noexcept;
  const mt::TechniqueConfiguration::Resource*
                                      _getResourceDescription() const noexcept;
  void _updateUniformValue();
  void _updateResource();
  void _updateSampler();
  void _makeUniformGUI();
  void _makeResourceGUI();
  void _makeTextureGUI();
  void _makeBufferGUI();
  SamplerValue& getSamplerValue();
  void _makeSamplerGUI();
  void _samplerModeGUI();
  void _customSamplerGUI();
  void _saveUniform(YAML::Emitter& target) const;
  void _saveResource(YAML::Emitter& target) const;
  void _saveSampler(YAML::Emitter& target) const;
  void _readUniform(const YAML::Node& source);
  void _readResource(const YAML::Node& source);
  void _readSampler(const YAML::Node& source);

private:
  mt::Technique& _technique;
  std::string _fullName;
  std::string _shortName;

  const TechniquePropsWidgetCommon& _commonData;

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

  std::unique_ptr<SamplerValue> _samplerValue;
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
