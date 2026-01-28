#include <hld/colorFrameBuilder/OpaqueColorStage.h>
#include <hld/drawCommand/DrawCommandList.h>
#include <hld/drawScene/Drawable.h>
#include <hld/DrawPlan.h>
#include <hld/HLDLib.h>
#include <technique/DescriptorSetType.h>
#include <util/Assert.h>
#include <vkr/image/ImageView.h>
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
                            const FrameBuildContext& frameContext)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  MT_ASSERT(_depthBuffer != nullptr);

  _drawCommands.clear();
  _commandMemoryPool.reset();

  if(_frameBuffer == nullptr) _buildFrameBuffer();

  for(const Drawable* drawable : drawPlan.stagePlan(_stageIndex))
  {
    MT_ASSERT(drawable->drawType() == Drawable::COMMANDS_DRAW);
    drawable->addToCommandList( _drawCommands,
                                frameContext,
                                _stageIndex,
                                nullptr);
  }

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                *_frameBuffer);

  _drawCommands.draw( commandProducer,
                      DrawCommandList::BY_GROUP_INDEX_SORTING);

  renderPass.endPass();
}

void OpaqueColorStage::_buildFrameBuffer()
{
  Ref<ImageView> colorTarget(
                            new ImageView(*_hdrBuffer,
                                          ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                                      0,
                                                      1,
                                                      0,
                                                      1),
                                          VK_IMAGE_VIEW_TYPE_2D));
  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = colorTarget.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};


  Ref<ImageView> depthTarget(
                        new ImageView(*_depthBuffer,
                                      ImageSlice( VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT,
                                                  0,
                                                  1,
                                                  0,
                                                  1),
                                      VK_IMAGE_VIEW_TYPE_2D));

  FrameBuffer::DepthStencilAttachmentInfo depthAttachment = {
                                        .target = depthTarget.get(),
                                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                        .clearValue = { .depth = 0,
                                                        .stencil = 0}};

  _frameBuffer = new FrameBuffer( std::span(&colorAttachment, 1),
                                  &depthAttachment);
}

