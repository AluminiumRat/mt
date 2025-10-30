#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Image.h>

using namespace mt;

CommandProducerGraphic::CommandProducerGraphic(CommandPoolSet& poolSet) :
  CommandProducerCompute(poolSet)
{
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

    std::pair<const Image*, const ImageAccess*> accesses[2] =
                                              { {&srcImage, &srcImageAccess},
                                                {&dstImage, &dstImageAccess}};
    addMultipleImagesUsage(accesses);
  }

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
