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

  //  Строит screen-space карту теней
  class ShadowsStage
  {
  public:
    explicit ShadowsStage(Device& device);
    ShadowsStage(const ShadowsStage&) = delete;
    ShadowsStage& operator = (const ShadowsStage&) = delete;
    ~ShadowsStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              const FrameBuildContext& frameContext);

    //  shadowBuffer - таргет-буфер для карты теней, в него будет рендериться
    //    результат. К моменту вызова должен находиться в лэйауте
    //    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    inline void setBuffers(const ImageView& shadowBuffer);

  private:
    void _buildFrameBuffer();

  private:
    Device& _device;

    Ref<TechniqueConfigurator> _resolveConfigurator;
    Technique _resolveTechnique;
    TechniquePass& _resolvePass;
    ResourceBinding& _tlasBinding;

    ConstRef<ImageView> _shadowBuffer;
    ConstRef<FrameBuffer> _resolveFrameBuffer;
  };

  inline void ShadowsStage::setBuffers(const ImageView& shadowBuffer)
  {
    if(_shadowBuffer == &shadowBuffer) return;
    _shadowBuffer = &shadowBuffer;
    _resolveFrameBuffer.reset();
  }
}