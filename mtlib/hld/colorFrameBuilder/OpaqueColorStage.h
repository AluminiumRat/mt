#pragma once

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class DrawPlan;
  struct FrameBuildContext;

  class OpaqueColorStage
  {
  public:
    static constexpr const char* stageName = "OpaqueColorStage";

  public:
    explicit OpaqueColorStage(Device& device);
    OpaqueColorStage(const OpaqueColorStage&) = delete;
    OpaqueColorStage& operator = (const OpaqueColorStage&) = delete;
    virtual ~OpaqueColorStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              const DrawPlan& drawPlan,
              const FrameBuildContext& frameContext,
              glm::uvec2 viewport);

    inline void setHdrBuffer(const ImageView& newBuffer) noexcept;
    inline void setDepthBuffer(const ImageView& newBuffer) noexcept;

  private:
    void _buildFrameBuffer();

  private:
    Device& _device;

    StageIndex _stageIndex;

    ConstRef<ImageView> _hdrBuffer;
    ConstRef<ImageView> _depthBuffer;
    ConstRef<FrameBuffer> _frameBuffer;

    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;
  };

  inline void OpaqueColorStage::setHdrBuffer(
                                            const ImageView& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }

  inline void OpaqueColorStage::setDepthBuffer(
                                            const ImageView& newBuffer) noexcept
  {
    if(_depthBuffer == &newBuffer) return;
    _depthBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }
}