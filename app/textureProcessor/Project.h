#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <asyncTask/AsyncTaskQueue.h>
#include <resourceManagement/FileWatcher.h>
#include <technique/Technique.h>
#include <techniquePropertySet/TechniquePropertySet.h>
#include <util/Ref.h>

class Project : mt::FileObserver
{
public:
  explicit Project(const std::filesystem::path& file);
  Project(const Project&) = delete;
  Project& operator = (const Project&) = delete;
  virtual ~Project() noexcept;

  inline const std::filesystem::path& projectFile() const;

  inline const mt::Image* resultImage() const noexcept;

  void save(const std::filesystem::path& file);
  void makeGui();

  void rebuildTechnique();
  void runTechnique();

protected:
  virtual void onFileChanged( const std::filesystem::path& filePath,
                              EventType eventType) override;

private:
  class BuildTextureTask;
  class RebuildTechniqueTask;

private:
  //  Вызывается из BuildTextureTask
  inline void setResultImage(mt::Image& image) noexcept;

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

  mt::Technique _technique;
  mt::TechniquePropertySet _techniqueProps;

  mt::Ref<mt::Image> _resultImage;

  std::unique_ptr<mt::AsyncTaskQueue::TaskHandle> _rebuildTaskHandle;
  std::unique_ptr<mt::AsyncTaskQueue::TaskHandle> _textureTaskHandle;
};

inline const std::filesystem::path& Project::projectFile() const
{
  return _projectFile;
}

inline const mt::Image* Project::resultImage() const noexcept
{
  return _resultImage.get();
}

inline void Project::setResultImage(mt::Image& image) noexcept
{
  _resultImage = mt::Ref(&image);
}

