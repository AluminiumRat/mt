#pragma once

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class TextureManager;

  //  Строит screen-space маску теней
  class ShadowsStage
  {
  public:
    ShadowsStage(Device& device, TextureManager& textureManager);
    ShadowsStage(const ShadowsStage&) = delete;
    ShadowsStage& operator = (const ShadowsStage&) = delete;
    ~ShadowsStage() noexcept = default;

    //  К моменту вызова shadowBuffer должен находиться в лэйауте
    //    VK_IMAGE_LAYOUT_GENERAL
    void draw(CommandProducerGraphic& commandProducer,
              const FrameBuildContext& frameContext);

    //  shadowBuffer - таргет-буфер для карты теней, в него будет уложена
    //    результирующая маска теней.
    //  Должен поддерживать использование в качестве storage буфера.
    //  Формат VK_FORMAT_R8_UNORM
    inline void setBuffers(const ImageView& shadowBuffer);

    //  Смещение луча в направлении солнца при трассировке
    inline float rayForwardShift() const noexcept;
    inline void setRayForwardShift(float newValue) noexcept;

    //  Смещение луча в направлении нормали при трассировке
    inline float rayNormalShift() const noexcept;
    inline void setRayNormalShift(float newValue) noexcept;

    void makeGui();

  private:
    void _createBuffers(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    Ref<TechniqueConfigurator> _rayQueryTechniqueConfigurator;
    Technique _rayQueryTechnique;
    TechniquePass& _rayQueryPass;
    TechniquePass& _spatialFilterPass;
    ResourceBinding& _tlasBinding;
    ResourceBinding& _noiseTextureBinding;
    ResourceBinding& _samplerTextureBinding;
    ResourceBinding& _rawShadowMaskBinding;
    ResourceBinding& _finalShadowMaskBinding;
    UniformVariable& _rayForwardShiftUniform;
    UniformVariable& _rayNormalShiftUniform;

    //  Смещения начала луча при рэй трэйсинге
    float _rayForwardShift;
    float _rayNormalShift;

    //  Буфер, куда кладутся сырые результаты трассировки теней
    ConstRef<ImageView> _rawShadowsBuffer;
    //  Буфер, куда кладется окончательно отфильтрованная маска теней
    ConstRef<ImageView> _shadowBuffer;

    //  Размеры сетки для компьют шейдеров
    glm::uvec2 _gridSize;
  };

  inline void ShadowsStage::setBuffers(const ImageView& shadowBuffer)
  {
    if(_shadowBuffer == &shadowBuffer) return;

    _shadowBuffer = &shadowBuffer;

    _rawShadowsBuffer.reset();
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