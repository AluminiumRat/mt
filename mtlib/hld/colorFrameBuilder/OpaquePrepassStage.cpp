#include <hld/colorFrameBuilder/OpaquePrepassStage.h>
#include <hld/drawScene/Drawable.h>
#include <hld/DrawPlan.h>
#include <hld/HLDLib.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

OpaquePrepassStage::OpaquePrepassStage(Device& device) :
  RegularDrawStage(device, stageName, DrawCommandList::BY_GROUP_INDEX_SORTING)
{
}

ConstRef<FrameBuffer> OpaquePrepassStage::buildFrameBuffer() const
{
  MT_ASSERT(_depthBuffer != nullptr);
  MT_ASSERT(_normalBuffer != nullptr);

  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = _normalBuffer.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};

  FrameBuffer::DepthStencilAttachmentInfo depthAttachment = {
                                        .target = _depthBuffer.get(),
                                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                        .clearValue = { .depth = 0,
                                                        .stencil = 0}};

  return ConstRef(new FrameBuffer(std::span(&colorAttachment, 1),
                                  &depthAttachment));
}
