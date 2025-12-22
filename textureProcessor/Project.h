#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class BaseWindow;
}

class Project
{
public:
  Project(const std::filesystem::path& file,
          const mt::BaseWindow& parentWindow);
  Project(const Project&) = delete;
  Project& operator = (const Project&) = delete;
  virtual ~Project() noexcept = default;

  inline const std::filesystem::path& projectFile() const;

  void save(const std::filesystem::path& file);
  void guiPass();

private:
  void _load();
  void _selectShader() noexcept;
  void _guiOutputProps() noexcept;
  void _selectOutputFile() noexcept;

private:
  std::filesystem::path _projectFile;

  const mt::BaseWindow& _parentWindow;

  std::filesystem::path _shaderFile;

  std::filesystem::path _outputFile;
  VkFormat _imageFormat;
  glm::ivec2 _outputSize;
  int32_t _mipsCount;
  int32_t _arraySize;

  mt::Ref<mt::TechniqueConfigurator> _configurator;
  mt::Ref<mt::Technique> _technique;
};

inline const std::filesystem::path& Project::projectFile() const
{
  return _projectFile;
}
