#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/Technique.h>
#include <techniquePropertyGrid/TechniquePropertyGrid.h>
#include <util/Ref.h>

class Project : mt::FileObserver
{
public:
  explicit Project(const std::filesystem::path& file);
  Project(const Project&) = delete;
  Project& operator = (const Project&) = delete;
  virtual ~Project() noexcept;

  inline const std::filesystem::path& projectFile() const;

  void save(const std::filesystem::path& file);
  void guiPass();

  void rebuildTechnique();
  void runTechnique();

protected:
  virtual void onFileChanged( const std::filesystem::path& filePath,
                              EventType eventType) override;

private:
  class RebuildTechniqueTask;

private:
  void _load();
  void _selectShader() noexcept;
  void _guiOutputProps();
  void _selectOutputFile() noexcept;

private:
  std::filesystem::path _projectFile;

  std::filesystem::path _shaderFile;

  std::filesystem::path _outputFile;
  VkFormat _imageFormat;
  glm::ivec2 _outputSize;
  int32_t _mipsCount;
  int32_t _arraySize;

  mt::Ref<mt::TechniqueConfigurator> _configurator;
  mt::Ref<mt::Technique> _technique;

  mt::TechniquePropertyGrid _propsGrid;

  std::unique_ptr<mt::AsyncTaskQueue::TaskHandle> _rebuildTaskHandle;
  std::unique_ptr<mt::AsyncTaskQueue::TaskHandle> _textureTaskHandle;
};

inline const std::filesystem::path& Project::projectFile() const
{
  return _projectFile;
}
