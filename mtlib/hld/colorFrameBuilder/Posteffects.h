#pragma once

#include <hld/colorFrameBuilder/BrightnessPyramid.h>
#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class TechniqueManager;

  //  Преобразование HDL в LDR и и пострендер
  class Posteffects
  {
  public:
    explicit Posteffects(Device& device);
    Posteffects(const Posteffects&) = delete;
    Posteffects& operator = (const Posteffects&) = delete;
    ~Posteffects() noexcept= default;

    //  Предварительная работа. Вызывается вне рендер паса
    //  hdr буфер в этот момент должен быть в лэйауте
    //  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    void prepare( CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    //  Конечный этап, создание изображения в LDR буфере
    //  Вызывается внутри рендер паса, где LDR прибижен как таргет
    //  hdr буфер в этот момент должен быть в лэйауте
    //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    void makeLDR( CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    inline void setHdrBuffer(Image& newBuffer) noexcept;

    void makeGui();

  private:
    void _updateBindings();

  private:
    Device& _device;

    Ref<Image> _hdrBuffer;
    bool _hdrBufferChanged;

    BrightnessPyramid _brightnessPyramid;

    Ref<TechniqueConfigurator> _resolveConfigurator;
    Technique _resolveTechnique;
    TechniquePass& _resolvePass;
    ResourceBinding& _hdrBufferBinding;
    ResourceBinding& _brightnessPyramidBinding;
    ResourceBinding& _avgColorBinding;

    UniformVariable& _exposureUniform;
    float _exposure;

    UniformVariable& _maxWhiteUniform;
    float _maxWhite;
  };

  inline void Posteffects::setHdrBuffer(Image& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _hdrBufferChanged = true;
  }
}