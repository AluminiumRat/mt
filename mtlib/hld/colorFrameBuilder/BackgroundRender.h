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

    inline void setBuffers( const ImageView& hdrBuffer,
                            const ImageView& depthBuffer);

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

  inline void BackgroundRender::setBuffers(
                                          const ImageView& hdrBuffer,
                                          const ImageView& depthBuffer)
  {
    if(_hdrBuffer == &hdrBuffer && _depthBuffer == &depthBuffer) return;
    _hdrBuffer = &hdrBuffer;
    _depthBuffer = &depthBuffer;
    _frameBuffer = nullptr;
  }
}