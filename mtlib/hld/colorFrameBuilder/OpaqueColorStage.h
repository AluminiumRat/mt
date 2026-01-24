#pragma once

#include <hld/drawCommand/CommandMemoryPool.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/FrameBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class DescriptorSet;
  class Device;
  class DrawPlan;
  class PipelineLayout;

  class OpaqueColorStage
  {
  public:
    static constexpr const char* stageName = "OpaqueColorStage";

  public:
    OpaqueColorStage(Device& device, FrameTypeIndex frameTypeIndex);
    OpaqueColorStage(const OpaqueColorStage&) = delete;
    OpaqueColorStage& operator = (const OpaqueColorStage&) = delete;
    virtual ~OpaqueColorStage() noexcept = default;

    void draw(CommandProducerGraphic& commandProducer,
              const DrawPlan& drawPlan,
              const DescriptorSet& commonDescriptorSet,
              const PipelineLayout& commonSetPipelineLayout);

    inline void setHdrBuffer(Image& newBuffer) noexcept;
    inline void setDepthBuffer(Image& newBuffer) noexcept;

  private:
    void _buildFrameBuffer();
    void _initBuffersLayout(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    FrameTypeIndex _frameTypeIndex;
    StageIndex _stageIndex;

    Ref<Image> _hdrBuffer;
    Ref<Image> _depthBuffer;
    Ref<FrameBuffer> _frameBuffer;

    CommandMemoryPool _commandMemoryPool;
    DrawCommandList _drawCommands;
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