#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <hld/drawScene/Drawable.h>
#include <hld/DrawPlan.h>
#include <hld/HLDLib.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

OpaqueColorStage::OpaqueColorStage(Device& device) :
  _device(device),
  _stageIndex(HLDLib::instance().getStageIndex(stageName)),
  _commandMemoryPool(4 * 1024),
  _drawCommands(_commandMemoryPool)
{
}

void OpaqueColorStage::draw(CommandProducerGraphic& commandProducer,
                            const DrawPlan& drawPlan,
                            const FrameBuildContext& frameContext,
                            glm::uvec2 viewport)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  MT_ASSERT(_depthBuffer != nullptr);
  MT_ASSERT(viewport.x != 0 && viewport.y != 0);

  _drawCommands.clear();
  _commandMemoryPool.reset();

  _drawCommands.fillFromStagePlan(drawPlan.stagePlan(_stageIndex),
                                  frameContext,
                                  _stageIndex,
                                  nullptr);

  if(_frameBuffer == nullptr) _buildFrameBuffer();

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                *_frameBuffer,
                                                std::nullopt,
                                                viewport);
    _drawCommands.draw( commandProducer,
                        DrawCommandList::BY_GROUP_INDEX_SORTING);
  renderPass.endPass();
}

void OpaqueColorStage::_buildFrameBuffer()
{
  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = _hdrBuffer.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};

  FrameBuffer::DepthStencilAttachmentInfo depthAttachment = {
                                        .target = _depthBuffer.get(),
                                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                        .clearValue = { .depth = 0,
                                                        .stencil = 0}};

  _frameBuffer = new FrameBuffer( std::span(&colorAttachment, 1),
                                  &depthAttachment);
}

