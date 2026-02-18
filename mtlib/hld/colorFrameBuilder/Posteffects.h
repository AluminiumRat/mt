#pragma once

#include <hld/colorFrameBuilder/AvgLum.h>
#include <hld/colorFrameBuilder/Bloom.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <util/Region.h>
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

    inline float brightness() const noexcept;
    inline void setBrightness(float newValue);

    inline float maxWhite() const noexcept;
    inline void setMaxWhite(float newValue);

    inline bool bloomEnabled() const noexcept;
    inline void setBloomEnabled(bool newValue);

    //  Преобразует hd в ldr и накладывает постэффекты. Результат кладет в
    //    target
    //  hdr буфер в этот момент должен быть в лэйауте
    //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void makeLDR( FrameBuffer& target,
                  const Region& drawRegion,
                  CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    inline void setHdrBuffer(const ImageView& newBuffer) noexcept;

    void makeGui();

  private:
    void _updateBindings();
    void _updateProperties();

  private:
    Device& _device;

    ConstRef<ImageView> _hdrBuffer;
    bool _needUpdateBindings;

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

    Selection& _bloomEnabledSelection;
    bool _bloomEnabled;
  };

  inline float Posteffects::brightness() const noexcept
  {
    return _brightness;
  }

  inline void Posteffects::setBrightness(float newValue)
  {
    if(_brightness == newValue) return;
    _brightness = newValue;
    _updateProperties();
  }

  inline float Posteffects::maxWhite() const noexcept
  {
    return _maxWhite;
  }

  inline void Posteffects::setMaxWhite(float newValue)
  {
    if(_maxWhite == newValue) return;
    _maxWhite = newValue;
    _updateProperties();
  }

  inline bool Posteffects::bloomEnabled() const noexcept
  {
    return _bloomEnabled;
  }

  inline void Posteffects::setBloomEnabled(bool newValue)
  {
    if(_bloomEnabled == newValue) return;
    _bloomEnabled = newValue;
    _needUpdateBindings = true;
    _updateProperties();
  }

  inline void Posteffects::setHdrBuffer(const ImageView& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _avgLum.setSourceImage(newBuffer);
    _bloom.setSourceImage(newBuffer);
    _needUpdateBindings = true;
  }
}