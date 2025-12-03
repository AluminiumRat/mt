#include <memory>

#include <vulkan/vulkan.h>
#include <dds.hpp>

#include <ddsSupport/ContentLoader.h>
#include <ddsSupport/ddsSupport.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/Device.h>

using namespace mt;

static void checkDDSImage(const dds::Image& ddsImage, const char* filename)
{
  if(ddsImage.numMips == 0)
  {
    throw std::runtime_error(std::string("Wrong image num mips") + filename);
  }
  if (ddsImage.arraySize == 0)
  {
    throw std::runtime_error(std::string("Wrong image array size") + filename);
  }
  if (ddsImage.width == 0 ||
      ddsImage.height == 0 ||
      ddsImage.depth == 0)
  {
    throw std::runtime_error(std::string("Wrong image size") + filename);
  }
}

static VkImageType getImageType(const dds::Image& ddsImage,
                                const char* filename)
{
  switch (ddsImage.dimension) {
  case dds::Texture1D:
    return VK_IMAGE_TYPE_1D;
  case dds::Texture2D:
    return VK_IMAGE_TYPE_2D;
  case dds::Texture3D:
    return VK_IMAGE_TYPE_3D;
  }
  throw std::runtime_error(std::string("Unsupported dimension") + filename);
}

static void uploadDataToImage(Image& targetImage,
                              const dds::Image& srcData,
                              Device& device,
                              CommandQueueTransfer& transferQueue)
{
  std::unique_ptr<CommandProducerTransfer> producer =
                                                  transferQueue.startCommands();

  if(!targetImage.isLayoutAutoControlEnabled())
  {
    producer->imageBarrier( targetImage,
                            ImageSlice(targetImage),
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            0,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0);
  }

  for (uint32_t mipIndex = 0; mipIndex < srcData.numMips; mipIndex++)
  {
    size_t dataSize = srcData.mipmaps[mipIndex].size();
    Ref<DataBuffer> uploadBuffer(new DataBuffer(device,
                                                dataSize,
                                                DataBuffer::UPLOADING_BUFFER));
    uploadBuffer->uploadData(srcData.mipmaps[mipIndex].data(), 0, dataSize);

    glm::uvec3 dstExtent = targetImage.extent(mipIndex);

    producer->copyFromBufferToImage(*uploadBuffer,
                                    0,
                                    0,
                                    0,
                                    targetImage,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    srcData.arraySize,
                                    mipIndex,
                                    glm::uvec3(0,0,0),
                                    dstExtent);
  }

  if(!targetImage.isLayoutAutoControlEnabled())
  {
    producer->imageBarrier( targetImage,
                            ImageSlice(targetImage),
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                              VK_ACCESS_SHADER_READ_BIT |
                              VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
  }

  transferQueue.submitCommands(std::move(producer));
}

Ref<Image> mt::loadDDS( const char* filename,
                        Device& device,
                        CommandQueueTransfer* transferQueue,
                        bool layoutAutocontrol)
{
  if(transferQueue == nullptr) transferQueue = &device.primaryQueue();

  ContentLoader& loader = ContentLoader::getContentLoader();
  std::vector<char> fileData = loader.loadData(filename);

  dds::Image ddsImage;
  if(dds::readImage((std::uint8_t*)fileData.data(),
                    fileData.size(),
                    &ddsImage) != dds::Success)
  {
    throw std::runtime_error(std::string("Unable to read ") + filename);
  }

  checkDDSImage(ddsImage, filename);
  VkImageType imageType = getImageType(ddsImage, filename);

  VkFormat imageFormat = dds::getVulkanFormat(ddsImage.format,
                                              ddsImage.supportsAlpha);
  ImageFormatFeatures formatDesc = getFormatFeatures(imageFormat);
  if(!formatDesc.isColor)
  {
    throw std::runtime_error(std::string("Only color formats are supported: ") + filename);
  }

  Ref<Image> image(new Image( device,
                              imageType,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              0,
                              imageFormat,
                              glm::uvec3( ddsImage.width,
                                          ddsImage.height,
                                          ddsImage.depth),
                              VK_SAMPLE_COUNT_1_BIT,
                              ddsImage.arraySize,
                              ddsImage.numMips,
                              layoutAutocontrol));

  uploadDataToImage(*image, ddsImage, device, *transferQueue);

  return image;
}