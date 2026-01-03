#pragma once

#include <glm/glm.hpp>

#include <technique/Technique.h>
#include <util/Camera.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImageView.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/FrameBuffer.h>
#include <vkr/Sampler.h>

#include <FlatCameraManipulator.h>

namespace mt
{
  class Device;
  class Image;
  class TechniqueManager;
}

class TextureViewer
{
public:
  TextureViewer(mt::Device& device);
  TextureViewer(const TextureViewer&) = delete;
  TextureViewer& operator = (const TextureViewer&) = delete;
  ~TextureViewer() noexcept;

  //  image должна либо иметь включенный автоконтроль лэйаута, либо находиться
  //    в лэйауте VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  void makeGUI( const char* id,
                const mt::Image& image,
                ImVec2 size = ImVec2(-FLT_MIN, -FLT_MIN));

private:
  enum SamplerType
  {
    NEAREST_SAMPLER,
    LINEAR_SAMPLER
  };

private:
  //  Виджеты управления сверху от основного окна просмотра
  void _makeControlWidgets();

  //  Создет область, над которой бедет работать манипулятор камеры
  void _makeUndraggedArea(glm::uvec2 widgetSize);

  // Определить реальный размер виджета по размеру, переданному пользователем
  glm::uvec2 _getWidgetSize(const ImVec2& userSize) const;

  void _rebuildRenderTarget(glm::uvec2 widgetSize);
  void _setNewRenderedImage(const mt::Image& image);
  void _clearRenderTarget() noexcept;

  void _updateRenderParams();
  void _renderScene();

private:
  mt::Device& _device;

  mt::Ref<mt::Technique> _viewTechnique;
  mt::ResourceBinding* _texture;
  mt::TechniquePass* _flatPass;
  mt::UniformVariable* _viewProjectionMatrix;
  mt::UniformVariable* _modelMatrix;
  mt::UniformVariable* _mipUniform;
  mt::UniformVariable* _layerUniform;
  mt::Selection* _samplerSelection;

  mt::ConstRef<mt::Image> _renderedImage;
  mt::ConstRef<mt::ImageView> _renderedImageView;

  mt::Ref<mt::Image> _renderTargetImage;
  mt::Ref<mt::FrameBuffer> _frameBuffer;

  mt::Camera _viewCamera;
  FlatCameraManipulator _cameraManipulator;

  SamplerType _samplerType;
  int _mipIndex;
  int _layerIndex;

  //  Объекты, которые нужны для отрисовки _renderTargetImage в окно ImGui
  mt::Ref<mt::Sampler> _imGuiSampler;
  mt::Ref<mt::DescriptorSet> _imGuiDescriptorSet;
};