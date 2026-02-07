#include <stdexcept>

#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/image/Image.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/FrameBuffer.h>

using namespace mt;

CommandProducerGraphic::CommandProducerGraphic( CommandPoolSet& poolSet,
                                                const char* debugName) :
  CommandProducerCompute(poolSet, debugName),
  _currentPass(nullptr)
{
}

CommandProducerGraphic::~CommandProducerGraphic() noexcept
{
  MT_ASSERT(_currentPass == nullptr);
}

void CommandProducerGraphic::finalizeCommands() noexcept
{
  if(_currentPass != nullptr)
  {
    Log::warning() << "CommandProducerGraphic::finalizeCommands: current pass is not finished";
    _endPass();
  }

  _graphicPipeline.reset();
  _graphicDescriptors.clear();

  CommandProducerCompute::finalizeCommands();
}

void CommandProducerGraphic::restoreBindings(CommandBuffer& buffer)
{
  if(_graphicPipeline != nullptr)
  {
    vkCmdBindPipeline(buffer.handle(),
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      _graphicPipeline->handle());
  }

  for(uint32_t setIndex = 0;
      setIndex < _graphicDescriptors.size();
      setIndex++)
  {
    const BindingRecord& binding = _graphicDescriptors[setIndex];
    if(binding.descriptors == nullptr) continue;
    MT_ASSERT(binding.layout != nullptr);

    VkDescriptorSet setHandle = binding.descriptors->handle();
    vkCmdBindDescriptorSets(buffer.handle(),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            binding.layout->handle(),
                            setIndex,
                            1,
                            &setHandle,
                            0,
                            nullptr);
  }

  CommandProducerCompute::restoreBindings(buffer);
}

void CommandProducerGraphic::_beginPass(RenderPass& renderPass)
{
  MT_ASSERT(_currentPass == nullptr);

  try
  {
    beginRenderPassBlock();

    const FrameBuffer& frameBuffer = renderPass.frameBuffer();

    lockResource(frameBuffer);
    addMultipleImagesUsage(frameBuffer.imagesAccess().accessTable());

    CommandBuffer& buffer = getOrCreateBuffer();

    vkCmdBeginRendering(buffer.handle(), &frameBuffer.bindingInfo());

    // Выставляем дефолтный вьюпорт
    VkViewport viewport{.x = 0,
                        .y = 0,
                        .width = float(frameBuffer.extent().x),
                        .height = float(frameBuffer.extent().y),
                        .minDepth = 0,
                        .maxDepth = 1};
    vkCmdSetViewport(buffer.handle(), 0, 1, &viewport);

    // Дефолтный трафарет
    VkRect2D scissor{ .offset = {.x = 0, .y = 0},
                      .extent = { .width = frameBuffer.extent().x,
                                  .height = frameBuffer.extent().y}};
    vkCmdSetScissor(buffer.handle(), 0, 1, &scissor);

    _currentPass = &renderPass;
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandProducerGraphic::_beginPass: " << error.what();
    Abort("Unable to begin render pass");
  }
}

void CommandProducerGraphic::_endPass() noexcept
{
  MT_ASSERT(_currentPass != nullptr);

  try
  {
    CommandBuffer& buffer = getOrCreateBuffer();
    vkCmdEndRendering(buffer.handle());
    _currentPass = nullptr;
    endRenderPassBlock();
  }
  catch(std::exception& error)
  {
    Log::error() << "CommandProducerGraphic::_endPass: " << error.what();
    Abort("Unable to end render pass");
  }
}

void CommandProducerGraphic::setViewport(Region region)
{
  if(_currentPass == nullptr) return;

  if(!region.valid()) region = curentFrameBuffer()->extent();
  MT_ASSERT(region.valid());

  CommandBuffer& buffer = getOrCreateBuffer();

  VkViewport viewport{.x = float(region.minCorner.x),
                      .y = float(region.minCorner.y),
                      .width = float(region.width()),
                      .height = float(region.height()),
                      .minDepth = 0,
                      .maxDepth = 1};
  vkCmdSetViewport(buffer.handle(), 0, 1, &viewport);
}

void CommandProducerGraphic::setScissor(Region region)
{
  if(_currentPass == nullptr) return;

  if(!region.valid()) region = curentFrameBuffer()->extent();
  MT_ASSERT(region.valid());

  CommandBuffer& buffer = getOrCreateBuffer();
  VkRect2D scissor{ .offset = { .x = int32_t(region.minCorner.x),
                                .y = int32_t(region.minCorner.y)},
                    .extent = { .width = region.width(),
                                .height = region.height()}};
  vkCmdSetScissor(buffer.handle(), 0, 1, &scissor);
}

void CommandProducerGraphic::bindGraphicPipeline(const GraphicPipeline& pipeline)
{
  if(&pipeline == _graphicPipeline.get()) return;

  lockResource(pipeline);

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindPipeline(buffer.handle(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.handle());

  _graphicPipeline = &pipeline;
}

void CommandProducerGraphic::unbindGraphicPipeline()
{
  _graphicPipeline = nullptr;
}

void CommandProducerGraphic::bindDescriptorSetGraphic(
                                            const DescriptorSet& descriptorSet,
                                            uint32_t setIndex,
                                            const PipelineLayout& layout)
{
  MT_ASSERT(descriptorSet.isFinalized());

  if(setIndex >= _graphicDescriptors.size())
  {
    _graphicDescriptors.resize(setIndex + 1);
  }
  if(_graphicDescriptors[setIndex].descriptors == &descriptorSet) return;

  lockResource(layout);
  lockResource(descriptorSet);

  addMultipleImagesUsage(descriptorSet.imagesAccess().accessTable());

  VkDescriptorSet setHandle = descriptorSet.handle();
  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindDescriptorSets(buffer.handle(),
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          layout.handle(),
                          setIndex,
                          1,
                          &setHandle,
                          0,
                          nullptr);

  _graphicDescriptors[setIndex] = BindingRecord{.layout{&layout},
                                                .descriptors{&descriptorSet}};
}

void CommandProducerGraphic::unbindDescriptorSetGraphic(
                                                    uint32_t setIndex) noexcept
{
  if(setIndex >= _graphicDescriptors.size()) return;
  _graphicDescriptors[setIndex] = BindingRecord{};
}

void CommandProducerGraphic::draw(uint32_t vertexCount,
                                  uint32_t instanceCount,
                                  uint32_t firstVertex,
                                  uint32_t firstInstance)
{
  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdDraw(buffer.handle(),
            vertexCount,
            instanceCount,
            firstVertex,
            firstInstance);
}

void CommandProducerGraphic::blitImage( const Image& srcImage,
                                        VkImageAspectFlags srcAspect,
                                        uint32_t srcArrayIndex,
                                        uint32_t srcMipLevel,
                                        const glm::uvec3& srcOffset,
                                        const glm::uvec3& srcExtent,
                                        const Image& dstImage,
                                        VkImageAspectFlags dstAspect,
                                        uint32_t dstArrayIndex,
                                        uint32_t dstMipLevel,
                                        const glm::uvec3& dstOffset,
                                        const glm::uvec3& dstExtent,
                                        VkFilter filter)
{
  // Для начала подготовим доступ к Image-ам
  if(&srcImage == &dstImage)
  {
    // Мы копируем внутри одного Image-а
    ImageAccess imageAccess;
    imageAccess.slices[0] = ImageSlice( srcImage,
                                        srcAspect,
                                        srcMipLevel,
                                        1,
                                        srcArrayIndex,
                                        1);
    imageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .readAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                              .writeStagesMask = 0,
                              .writeAccessMask = 0};

    imageAccess.slices[1] = ImageSlice( dstImage,
                                        dstAspect,
                                        dstMipLevel,
                                        1,
                                        dstArrayIndex,
                                        1);
    imageAccess.layouts[1] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageAccess.memoryAccess[1] = MemoryAccess{
                              .readStagesMask = 0,
                              .readAccessMask = 0,
                              .writeStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .writeAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT};
    imageAccess.slicesCount = 2;
    addImageUsage(srcImage, imageAccess);
  }
  else
  {
    // Два разных Image
    ImageAccess srcImageAccess;
    srcImageAccess.slices[0] = ImageSlice(srcImage,
                                          srcAspect,
                                          srcMipLevel,
                                          1,
                                          srcArrayIndex,
                                          1);
    srcImageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    srcImageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .readAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                              .writeStagesMask = 0,
                              .writeAccessMask = 0};
    srcImageAccess.slicesCount = 1;

    ImageAccess dstImageAccess;
    dstImageAccess.slices[0] = ImageSlice(dstImage,
                                          dstAspect,
                                          dstMipLevel,
                                          1,
                                          dstArrayIndex,
                                          1);
    dstImageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dstImageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = 0,
                              .readAccessMask = 0,
                              .writeStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .writeAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT};
    dstImageAccess.slicesCount = 1;

    std::pair<const Image*, ImageAccess> accesses[2] =
                                              { {&srcImage, srcImageAccess},
                                                {&dstImage, dstImageAccess}};
    addMultipleImagesUsage(accesses);
  }

  lockResource(srcImage);
  lockResource(dstImage);

  // Дальше собственно сама операция блита
  VkImageBlit region{};
  region.srcSubresource.aspectMask = srcAspect;
  region.srcSubresource.mipLevel = srcMipLevel;
  region.srcSubresource.baseArrayLayer = srcArrayIndex;
  region.srcSubresource.layerCount = 1;
  region.srcOffsets[0].x = uint32_t(srcOffset.x);
  region.srcOffsets[0].y = uint32_t(srcOffset.y);
  region.srcOffsets[0].z = uint32_t(srcOffset.z);
  region.srcOffsets[1].x = uint32_t(srcOffset.x + srcExtent.x);
  region.srcOffsets[1].y = uint32_t(srcOffset.y + srcExtent.y);
  region.srcOffsets[1].z = uint32_t(srcOffset.z + srcExtent.z);

  region.dstSubresource.aspectMask = dstAspect;
  region.dstSubresource.mipLevel = dstMipLevel;
  region.dstSubresource.baseArrayLayer = dstArrayIndex;
  region.dstSubresource.layerCount = 1;
  region.dstOffsets[0].x = uint32_t(dstOffset.x);
  region.dstOffsets[0].y = uint32_t(dstOffset.y);
  region.dstOffsets[0].z = uint32_t(dstOffset.z);
  region.dstOffsets[1].x = uint32_t(dstOffset.x + dstExtent.x);
  region.dstOffsets[1].y = uint32_t(dstOffset.y + dstExtent.y);
  region.dstOffsets[1].z = uint32_t(dstOffset.z + dstExtent.z);

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBlitImage( buffer.handle(),
                  srcImage.handle(),
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  dstImage.handle(),
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  1,
                  &region,
                  filter);
}
