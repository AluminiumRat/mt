#pragma once

#include <hld/colorFrameBuilder/AvgLum.h>
#include <hld/colorFrameBuilder/Bloom.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class TechniqueManager;

  //  Преобразование HDL в LDR и пострендер
  class Posteffects
  {
  public:
    explicit Posteffects(Device& device);
    Posteffects(const Posteffects&) = delete;
    Posteffects& operator = (const Posteffects&) = delete;
    ~Posteffects() noexcept= default;

    //  Преобразует hd в ldr и накладывает постэффекты. Результат кладет в
    //    target
    //  hdr буфер в этот момент должен быть в лэйауте
    //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void makeLDR( FrameBuffer& target,
                  CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    inline void setHdrBuffer(ImageView& newBuffer) noexcept;

    void makeGui();

  private:
    void _updateBindings();

  private:
    Device& _device;

    Ref<ImageView> _hdrBuffer;
    bool _hdrBufferChanged;

    AvgLum _avgLum;
    Bloom _bloom;

    Ref<TechniqueConfigurator> _resolveConfigurator;
    Technique _resolveTechnique;
    TechniquePass& _resolvePass;
    ResourceBinding& _hdrBufferBinding;
    ResourceBinding& _avgLuminanceBinding;
    ResourceBinding& _bloomTextureBinding;

    UniformVariable& _brightnessUniform;
    float _brightness;

    UniformVariable& _maxWhiteUniform;
    float _maxWhite;
  };

  inline void Posteffects::setHdrBuffer(ImageView& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _avgLum.setSourceImage(newBuffer);
    _bloom.setSourceImage(newBuffer);
    _hdrBufferChanged = true;
  }
}