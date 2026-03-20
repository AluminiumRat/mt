#pragma once

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class TextureManager;

  //  Строит screen-space карту теней
  class ShadowsStage
  {
  public:
    ShadowsStage(Device& device, TextureManager& textureManager);
    ShadowsStage(const ShadowsStage&) = delete;
    ShadowsStage& operator = (const ShadowsStage&) = delete;
    ~ShadowsStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              const FrameBuildContext& frameContext);

    //  shadowBuffer - таргет-буфер для карты теней, в него будет рендериться
    //    результат. К моменту вызова должен находиться в лэйауте
    //    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    inline void setBuffers(const ImageView& shadowBuffer);

    //  Смещение луча в направлении солнца при трассировке
    inline float rayForwardShift() const noexcept;
    inline void setRayForwardShift(float newValue) noexcept;

    //  Смещение луча в направлении нормали при трассировке
    inline float rayNormalShift() const noexcept;
    inline void setRayNormalShift(float newValue) noexcept;

    void makeGui();

  private:
    void _buildFrameBuffer();

  private:
    Device& _device;

    Ref<TechniqueConfigurator> _rayQueryTechniqueConfigurator;
    Technique _rayQueryTechnique;
    TechniquePass& _rayQueryPass;
    ResourceBinding& _tlasBinding;
    ResourceBinding& _noiseTextureBinding;
    ResourceBinding& _samplerTextureBinding;
    UniformVariable& _rayForwardShiftUniform;
    UniformVariable& _rayNormalShiftUniform;

    //  Смещения начала луча при рэй трэйсинге
    float _rayForwardShift;
    float _rayNormalShift;

    ConstRef<ImageView> _shadowBuffer;
    ConstRef<FrameBuffer> _resolveFrameBuffer;
  };

  inline void ShadowsStage::setBuffers(const ImageView& shadowBuffer)
  {
    if(_shadowBuffer == &shadowBuffer) return;
    _shadowBuffer = &shadowBuffer;
    _resolveFrameBuffer.reset();
  }

  inline float ShadowsStage::rayForwardShift() const noexcept
  {
    return _rayForwardShift;
  }

  inline void ShadowsStage::setRayForwardShift(float newValue) noexcept
  {
    if(newValue == _rayForwardShift) return;
    _rayForwardShift = newValue;
    _rayForwardShiftUniform.setValue(newValue);
  }

  inline float ShadowsStage::rayNormalShift() const noexcept
  {
    return _rayNormalShift;
  }

  inline void ShadowsStage::setRayNormalShift(float newValue) noexcept
  {
    if(newValue == _rayNormalShift) return;
    _rayNormalShift = newValue;
    _rayNormalShiftUniform.setValue(newValue);
  }
}