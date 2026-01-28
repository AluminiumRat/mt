#pragma once

#include <hld/colorFrameBuilder/BrightnessPyramid.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class TechniqueManager;

  class Posteffects
  {
  public:
    Posteffects(Device& device, TechniqueManager& techniqueManager);
    Posteffects(const Posteffects&) = delete;
    Posteffects& operator = (const Posteffects&) = delete;
    ~Posteffects() noexcept= default;

    //  Предварительная работа. Вызывается вне рендер паса
    void prepare( CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    //  Конечный этап, создание изображения в LDR буфере
    //  Вызывается внутри рендер паса, где LDR прибижен как таргет
    void makeLDR( CommandProducerGraphic& commandProducer,
                  const FrameBuildContext& frameContext);

    inline void setHdrBuffer(Image& newBuffer) noexcept;

  private:
    Device& _device;
    TechniqueManager& _techniqueManager;

    Ref<Image> _hdrBuffer;
    BrightnessPyramid _brightnessPyramid;
  };

  inline void Posteffects::setHdrBuffer(Image& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
  }
}