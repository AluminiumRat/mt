#pragma once

#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  struct ColorFrameContext;
  class Device;

  class OpaqueColorStage
  {
  public:
    static constexpr const char* stageName = "OpaqueColorStage";

  public:
    explicit OpaqueColorStage(Device& device);
    OpaqueColorStage(const OpaqueColorStage&) = delete;
    OpaqueColorStage& operator = (const OpaqueColorStage&) = delete;
    virtual ~OpaqueColorStage() noexcept = default;

    void draw(ColorFrameContext& frameContext);

    inline void setHdrBuffer(Image& newBuffer) noexcept;
    inline void setDepthBuffer(Image& newBuffer) noexcept;

  private:
    void _buildFrameBuffer();
    void _initBuffersLayout(ColorFrameContext& frameContext);

  private:
    Device& _device;

    StageIndex _stageIndex;

    Ref<Image> _hdrBuffer;
    Ref<Image> _depthBuffer;
    Ref<FrameBuffer> _frameBuffer;
  };

  inline void OpaqueColorStage::setHdrBuffer(Image& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }

  inline void OpaqueColorStage::setDepthBuffer(Image& newBuffer) noexcept
  {
    if(_depthBuffer == &newBuffer) return;
    _depthBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }
}