#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <asyncTask/AsyncTask.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>

class BuildTextureTask : public mt::AsyncTask
{
public:
  BuildTextureTask( const mt::Technique& technique,
                    const std::filesystem::path& outputFile,
                    VkFormat textureFormat,
                    glm::uvec2 textureSize,
                    uint32_t mipsCount,
                    uint32_t arraySize);
  BuildTextureTask(const BuildTextureTask&) = delete;
  BuildTextureTask& operator = (const BuildTextureTask&) = delete;
  virtual ~BuildTextureTask() noexcept = default;

protected:
  virtual void asyncPart() override;
  virtual void finalizePart() override;

private:
  void _copyTechnique(const mt::Technique& technique);
  void _buildSlice(uint32_t mipIndex, uint32_t arrayIndex);
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

  //  Промежуточная текстура в формате VK_FORMAT_R32G32B32A32_SFLOAT
  //  В неё рендерим, используя настроенную технику, потом используем для
  //    конвертации в конечный формат и для отрисовки во вьювер
  mt::Ref<mt::Image> _targetImage;

  //  Текстура, переведенная в конечный формат, данные из неё попадут в
  //  конечный файл
  mt::Ref<mt::Image> _savedImage;
};