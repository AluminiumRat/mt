#pragma once

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class DescriptorSet;
  class Device;
  class DrawPlan;
  struct FrameBuildContext;
  class PipelineLayout;

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
              const FrameBuildContext& frameContext);

    inline void setHdrBuffer(ImageView& newBuffer) noexcept;
    inline void setDepthBuffer(ImageView& newBuffer) noexcept;

  private:
    void _buildFrameBuffer();

  private:
    Device& _device;

    StageIndex _stageIndex;

    Ref<ImageView> _hdrBuffer;
    Ref<ImageView> _depthBuffer;
    Ref<FrameBuffer> _frameBuffer;

    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;
  };

  inline void OpaqueColorStage::setHdrBuffer(ImageView& newBuffer) noexcept
  {
    if(_hdrBuffer == &newBuffer) return;
    _hdrBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }

  inline void OpaqueColorStage::setDepthBuffer(ImageView& newBuffer) noexcept
  {
    if(_depthBuffer == &newBuffer) return;
    _depthBuffer = &newBuffer;
    _frameBuffer = nullptr;
  }
}