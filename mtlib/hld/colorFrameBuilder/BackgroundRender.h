#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class TechniqueManager;

  //  Отрисовывает задник сцены
  class BackgroundRender
  {
  public:
    BackgroundRender(Device& device, TechniqueManager& techniqueManager);
    BackgroundRender(const BackgroundRender&) = delete;
    BackgroundRender& operator = (const BackgroundRender&) = delete;
    ~BackgroundRender() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              glm::uvec2 viewport);

    inline void setHdrBuffer(const ImageView& newBuffer) noexcept;
    inline void setDepthBuffer(const ImageView& newBuffer) noexcept;

  private:
    void _buildFrameBuffer();

  private:
    Device& _device;

    std::unique_ptr<Technique> _envmapTechnique;
    TechniquePass& _envmapPass;

    ConstRef<ImageView> _hdrBuffer;
    ConstRef<ImageView> _depthBuffer;
    ConstRef<FrameBuffer> _frameBuffer;
  };

  inline void BackgroundRender::setHdrBuffer(
                                            const ImageView& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }

  inline void BackgroundRender::setDepthBuffer(
                                            const ImageView& newBuffer) noexcept
  {
    if(_depthBuffer == &newBuffer) return;
    _depthBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }
}