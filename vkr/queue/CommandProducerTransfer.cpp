#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandProducerTransfer.h>
#include <vkr/Image.h>

using namespace mt;

CommandProducerTransfer::CommandProducerTransfer(CommandPoolSet& poolSet) :
  CommandProducer(poolSet)
{
}

void CommandProducerTransfer::copyFromBufferToImage(
                                            const DataBuffer& srcBuffer,
                                            VkDeviceSize srcBufferOffset,
                                            uint32_t srcRowLength,
                                            uint32_t srcImageHeight,
                                            const Image& dstImage,
                                            VkImageAspectFlags dstAspectMask,
                                            uint32_t dstArrayIndex,
                                            uint32_t dstMipLevel,
                                            glm::uvec3 dstOffset,
                                            glm::uvec3 dstExtent)
{
  ImageAccess imageAccess;
  imageAccess.slices[0] = ImageSlice( dstImage,
                                      dstAspectMask,
                                      dstMipLevel,
                                      1,
                                      dstArrayIndex,
                                      1);
  imageAccess.layouts[0] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageAccess.memoryAccess[0] = MemoryAccess{
                              .readStagesMask = 0,
                              .readAccessMask = 0,
                              .writeStagesMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                              .writeAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT};
  imageAccess.slicesCount = 1;
  addImageUsage(dstImage, imageAccess);
  addBufferUsage(srcBuffer);

  VkBufferImageCopy region{};
  region.bufferOffset = srcBufferOffset;
  region.bufferRowLength = srcRowLength;
  region.bufferImageHeight = srcImageHeight;

  region.imageSubresource.aspectMask = dstAspectMask;
  region.imageSubresource.mipLevel = dstMipLevel;
  region.imageSubresource.baseArrayLayer = dstArrayIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset.x = uint32_t(dstOffset.x);
  region.imageOffset.y = uint32_t(dstOffset.y);
  region.imageOffset.z = uint32_t(dstOffset.z);

  region.imageExtent.width = uint32_t(dstExtent.x);
  region.imageExtent.height = uint32_t(dstExtent.y);
  region.imageExtent.depth = uint32_t(dstExtent.z);

  CommandBuffer& buffer = getOrCreateBuffer();

  vkCmdCopyBufferToImage( buffer.handle(),
                          srcBuffer.handle(),
                          dstImage.handle(),
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1,
                          &region);
}

void CommandProducerTransfer::copyFromImageToBuffer(
                                            const Image& srcImage,
                                            VkImageAspectFlags srcAspectMask,
                                            uint32_t srcArrayIndex,
                                            uint32_t srcMipLevel,
                                            glm::uvec3 srcOffset,
                                            glm::uvec3 srcExtent,
                                            const DataBuffer& dstBuffer,
                                            VkDeviceSize dstBufferOffset,
                                            uint32_t dstRowLength,
                                            uint32_t dstImageHeight)
{
  ImageAccess imageAccess;
  imageAccess.slices[0] = ImageSlice( srcImage,
                                      srcAspectMask,
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
  imageAccess.slicesCount = 1;
  addImageUsage(srcImage, imageAccess);
  addBufferUsage(dstBuffer);

  VkBufferImageCopy region{};
  region.bufferOffset = dstBufferOffset;
  region.bufferRowLength = dstRowLength;
  region.bufferImageHeight = dstImageHeight;

  region.imageSubresource.aspectMask = srcAspectMask;
  region.imageSubresource.mipLevel = srcMipLevel;
  region.imageSubresource.baseArrayLayer = srcArrayIndex;
  region.imageSubresource.layerCount = 1;

  region.imageOffset.x = uint32_t(srcOffset.x);
  region.imageOffset.y = uint32_t(srcOffset.y);
  region.imageOffset.z = uint32_t(srcOffset.z);

  region.imageExtent.width = uint32_t(srcExtent.x);
  region.imageExtent.height = uint32_t(srcExtent.y);
  region.imageExtent.depth = uint32_t(srcExtent.z);

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdCopyImageToBuffer( buffer.handle(),
                          srcImage.handle(),
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          dstBuffer.handle(),
                          1,
                          &region);
}
