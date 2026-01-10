#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <asyncTask/AsyncTask.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/FrameBuffer.h>

#include <Project.h>

namespace mt
{
  class CommandProducerGraphic;
}

//  Таска, которая выполняет основное назначение процессора.
//  Для выбранных настроек запускает технику и рендерит во все слайсы
//  текстуры. После этого сохранет тексуру в файл и отдает
//  текстуру для просмотра в GUI
class Project::BuildTextureTask : public mt::AsyncTask
{
public:
  //  outputFile может быть пустым, в этом случае результат сохраняться не будет
  //  mipsCount может быть больше, чем надо, таска сама обрежет количество
  //    мипов до нужного
  BuildTextureTask( const mt::Technique& technique,
                    const std::filesystem::path& outputFile,
                    VkFormat textureFormat,
                    glm::uvec2 textureSize,
                    uint32_t mipsCount,
                    uint32_t arraySize,
                    Project& project);
  BuildTextureTask(const BuildTextureTask&) = delete;
  BuildTextureTask& operator = (const BuildTextureTask&) = delete;
  virtual ~BuildTextureTask() noexcept = default;

protected:
  virtual void asyncPart() override;
  virtual void finalizePart() override;

private:
  //  Создаем копию переданной техники, чтобы избежать конфликтов
  //  в многопотоке
  void _copyTechnique(const mt::Technique& technique);
  void _createTargetImage();
  void _buildSlice(uint32_t mipIndex, uint32_t arrayIndex);
  void _adjustIntrinsic(uint32_t mipIndex, uint32_t arrayIndex);
  mt::Ref<mt::FrameBuffer> _createFrameBuffer(uint32_t mipIndex,
                                              uint32_t arrayIndex);
  void _saveTexture();

private:
  std::filesystem::path _outputFile;
  VkFormat _textureFormat;
  glm::uvec2 _textureSize;
  uint32_t _mipsCount;
  uint32_t _arraySize;

  //  Копия оригенальной техники. Оригенальную технику использовать нельзя, так
  //    как она может перенастраиваться в основном потоке
  mt::Ref<mt::Technique> _technique;

  mt::Ref<mt::Image> _targetImage;

  Project& _project;
};