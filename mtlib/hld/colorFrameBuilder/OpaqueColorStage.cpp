#include <hld/FrameContext.h>
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

OpaqueColorStage::OpaqueColorStage( Device& device,
                                    FrameTypeIndex frameTypeIndex) :
  _device(device),
  _frameTypeIndex(frameTypeIndex),
  _stageIndex(HLDLib::instance().getStageIndex(stageName)),
  _commandMemoryPool(4 * 1024),
  _drawCommands(_commandMemoryPool)
{
}

void OpaqueColorStage::draw(FrameContext& frameContext,
                            const DescriptorSet& commonDescriptorSet,
                            const PipelineLayout& commonSetPipelineLayout)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  MT_ASSERT(_depthBuffer != nullptr);

  _drawCommands.clear();
  _commandMemoryPool.reset();

  frameContext.commandProducer->beginDebugLabel(stageName);
    _initBuffersLayout(*frameContext.commandProducer);

    if(_frameBuffer == nullptr) _buildFrameBuffer();

    const std::vector<const Drawable*>& drawables =
                                  frameContext.drawPlan->stagePlan(_stageIndex);
    for(const Drawable* drawable : drawables)
    {
      MT_ASSERT(drawable->drawType() == Drawable::COMMANDS_DRAW);
      drawable->addToCommandList( _drawCommands,
                                  _frameTypeIndex,
                                  _stageIndex,
                                  nullptr);
    }

    CommandProducerGraphic::RenderPass renderPass(*frameContext.commandProducer,
                                                  *_frameBuffer);

    frameContext.commandProducer->bindDescriptorSetGraphic(
                                            commonDescriptorSet,
                                            (uint32_t)DescriptorSetType::COMMON,
                                            commonSetPipelineLayout);

    _drawCommands.draw( *frameContext.commandProducer,
                        DrawCommandList::BY_GROUP_INDEX_SORTING);

    frameContext.commandProducer->unbindDescriptorSetGraphic(
                                          (uint32_t)DescriptorSetType::COMMON);

    renderPass.endPass();

  frameContext.commandProducer->endDebugLabel();
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

void OpaqueColorStage::_initBuffersLayout(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_hdrBuffer,
                                ImageSlice(*_hdrBuffer),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                0,
                                0,
                                0,
                                0);

  commandProducer.imageBarrier(
                              *_depthBuffer,
                              ImageSlice(*_depthBuffer),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                              0,
                              0,
                              0,
                              0);
}
