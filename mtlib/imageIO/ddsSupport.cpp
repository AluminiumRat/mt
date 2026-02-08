#include <fstream>

#include <vulkan/vulkan.h>
#include <dds.hpp>

#include <imageIO/imageIO.h>
#include <util/ContentLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/Device.h>

namespace fs = std::filesystem;

using namespace mt;

static void checkDDSImage(const dds::Image& ddsImage,
                          const std::string& filename)
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
                                const std::string& filename)
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

  for (uint32_t layerIndex = 0; layerIndex < srcData.arraySize; layerIndex++)
  {
    for (uint32_t mipIndex = 0; mipIndex < srcData.numMips; mipIndex++)
    {
      size_t dataSize = srcData.layers[layerIndex][mipIndex].size();
      Ref<DataBuffer> uploadBuffer(new DataBuffer(device,
                                                  dataSize,
                                                  DataBuffer::UPLOADING_BUFFER,
                                                  "DDS uploading buffer"));
      uploadBuffer->uploadData( srcData.layers[layerIndex][mipIndex].data(),
                                0,
                                dataSize);

      glm::uvec3 dstExtent = targetImage.extent(mipIndex);

      producer->copyFromBufferToImage(*uploadBuffer,
                                      0,
                                      0,
                                      0,
                                      targetImage,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      layerIndex,
                                      1,
                                      mipIndex,
                                      glm::uvec3(0,0,0),
                                      dstExtent);
    }
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

Ref<Image> mt::loadDDS( const fs::path& file,
                        Device& device,
                        CommandQueueTransfer* transferQueue,
                        bool layoutAutocontrol)
{
  if(transferQueue == nullptr) transferQueue = &device.primaryQueue();

  ContentLoader& loader = ContentLoader::getLoader();
  std::vector<char> fileData = loader.loadData(file);

  std::string filename = (const char*)file.u8string().c_str();

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
                              layoutAutocontrol,
                              filename.c_str()));

  uploadDataToImage(*image, ddsImage, device, *transferQueue);

  return image;
}

static std::vector<Ref<DataBuffer>> readSlices(
                                            const Image& srcImage,
                                            Device& device,
                                            CommandQueueTransfer& transferQueue)
{
  VkFormat format = srcImage.format();
  const ImageFormatFeatures& formatDesc = getFormatFeatures(format);

  if(!formatDesc.isColor) throw std::runtime_error("mt::saveToDDS: unsupported srcImage format");

  std::unique_ptr<CommandProducerTransfer> producer =
                                                  transferQueue.startCommands();
  // Скачиваем мипы
  std::vector<Ref<DataBuffer>> buffers;
  for(uint32_t arrayIndex = 0; arrayIndex < srcImage.arraySize(); arrayIndex++)
  {
    for(uint32_t mipLevel = 0; mipLevel < srcImage.mipmapCount(); mipLevel++)
    {
      glm::uvec3 mipExtent = srcImage.extent(mipLevel);
      if(formatDesc.isCompressed)
      {
        mipExtent.x += (4 - mipExtent.x % 4) % 4;
        mipExtent.y += (4 - mipExtent.y % 4) % 4;
      }
      size_t mipDataSize = mipExtent.x * mipExtent.y * mipExtent.z *
                                                          formatDesc.texelSize;
      mipDataSize = (mipDataSize + 7) / 8;

      Ref<DataBuffer> mipBuffer(new DataBuffer( device,
                                                mipDataSize,
                                                DataBuffer::DOWNLOADING_BUFFER,
                                                "DDS downloading buffer"));
      producer->copyFromImageToBuffer(srcImage,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      arrayIndex,
                                      1,
                                      mipLevel,
                                      glm::uvec3(0),
                                      srcImage.extent(mipLevel),
                                      *mipBuffer,
                                      0,
                                      0,
                                      0);
      buffers.push_back(mipBuffer);
    }
  }

  transferQueue.submitCommands(std::move(producer));
  transferQueue.waitIdle();

  return buffers;
}

DXGI_FORMAT getDXGIFormat(const ImageFormatFeatures& vkFormat)
{
  switch(vkFormat.format)
  {
  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    return DXGI_FORMAT_BC1_UNORM;
  case VK_FORMAT_BC2_UNORM_BLOCK:
    return DXGI_FORMAT_BC2_UNORM;
  case VK_FORMAT_BC2_SRGB_BLOCK:
    return DXGI_FORMAT_BC2_UNORM_SRGB;
  case VK_FORMAT_BC3_UNORM_BLOCK:
    return DXGI_FORMAT_BC3_UNORM;
  case VK_FORMAT_BC3_SRGB_BLOCK:
    return DXGI_FORMAT_BC3_UNORM_SRGB;
  case VK_FORMAT_BC4_UNORM_BLOCK:
    return DXGI_FORMAT_BC4_UNORM;
  case VK_FORMAT_BC4_SNORM_BLOCK:
    return DXGI_FORMAT_BC4_SNORM;
  case VK_FORMAT_BC5_UNORM_BLOCK:
    return DXGI_FORMAT_BC5_UNORM;
  case VK_FORMAT_BC5_SNORM_BLOCK:
    return DXGI_FORMAT_BC5_SNORM;
  case VK_FORMAT_BC7_UNORM_BLOCK:
    return DXGI_FORMAT_BC7_UNORM;
  case VK_FORMAT_BC7_SRGB_BLOCK:
    return DXGI_FORMAT_BC7_UNORM_SRGB;
  case VK_FORMAT_R8_UNORM:
    return DXGI_FORMAT_R8_UNORM;
  case VK_FORMAT_R8_UINT:
    return DXGI_FORMAT_R8_UINT;
  case VK_FORMAT_R8_SNORM:
    return DXGI_FORMAT_R8_SNORM;
  case VK_FORMAT_R8_SINT:
    return DXGI_FORMAT_R8_SINT;
  case VK_FORMAT_A8_UNORM_KHR:
    return DXGI_FORMAT_A8_UNORM;
  case VK_FORMAT_R8G8_UNORM:
    return DXGI_FORMAT_R8G8_UNORM;
  case VK_FORMAT_R8G8_UINT:
    return DXGI_FORMAT_R8G8_UINT;
  case VK_FORMAT_R8G8_SNORM:
    return DXGI_FORMAT_R8G8_SNORM;
  case VK_FORMAT_R8G8_SINT:
    return DXGI_FORMAT_R8G8_SINT;
  case VK_FORMAT_R16_SFLOAT:
    return DXGI_FORMAT_R16_FLOAT;
  case VK_FORMAT_R16_UNORM:
    return DXGI_FORMAT_R16_UNORM;
  case VK_FORMAT_R16_UINT:
    return DXGI_FORMAT_R16_UINT;
  case VK_FORMAT_R16_SNORM:
    return DXGI_FORMAT_R16_SNORM;
  case VK_FORMAT_R16_SINT:
    return DXGI_FORMAT_R16_SINT;
  case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
    return DXGI_FORMAT_B5G5R5A1_UNORM;
  case VK_FORMAT_B5G6R5_UNORM_PACK16:
    return DXGI_FORMAT_B5G6R5_UNORM;
  case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
    return DXGI_FORMAT_B4G4R4A4_UNORM;
  case VK_FORMAT_R8G8B8A8_UNORM:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case VK_FORMAT_R8G8B8A8_SRGB:
    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  case VK_FORMAT_R8G8B8A8_UINT:
    return DXGI_FORMAT_R8G8B8A8_UINT;
  case VK_FORMAT_R8G8B8A8_SNORM:
    return DXGI_FORMAT_R8G8B8A8_SNORM;
  case VK_FORMAT_R8G8B8A8_SINT:
    return DXGI_FORMAT_R8G8B8A8_SINT;
  case VK_FORMAT_B8G8R8A8_UNORM:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case VK_FORMAT_B8G8R8A8_SRGB:
    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
  case VK_FORMAT_R16G16_SFLOAT:
    return DXGI_FORMAT_R16G16_FLOAT;
  case VK_FORMAT_R16G16_UNORM:
    return DXGI_FORMAT_R16G16_UNORM;
  case VK_FORMAT_R16G16_UINT:
    return DXGI_FORMAT_R16G16_UINT;
  case VK_FORMAT_R16G16_SNORM:
    return DXGI_FORMAT_R16G16_SNORM;
  case VK_FORMAT_R16G16_SINT:
    return DXGI_FORMAT_R16G16_SINT;
  case VK_FORMAT_R32_SFLOAT:
    return DXGI_FORMAT_R32_FLOAT;
  case VK_FORMAT_R32_UINT:
    return DXGI_FORMAT_R32_UINT;
  case VK_FORMAT_R32_SINT:
    return DXGI_FORMAT_R32_SINT;
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    return DXGI_FORMAT_R10G10B10A2_UINT;
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    return DXGI_FORMAT_R11G11B10_FLOAT;
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case VK_FORMAT_R16G16B16A16_SINT:
    return DXGI_FORMAT_R16G16B16A16_SINT;
  case VK_FORMAT_R16G16B16A16_UINT:
    return DXGI_FORMAT_R16G16B16A16_UINT;
  case VK_FORMAT_R16G16B16A16_UNORM:
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  case VK_FORMAT_R16G16B16A16_SNORM:
    return DXGI_FORMAT_R16G16B16A16_SNORM;
  case VK_FORMAT_R32G32_SFLOAT:
    return DXGI_FORMAT_R32G32_FLOAT;
  case VK_FORMAT_R32G32_UINT:
    return DXGI_FORMAT_R32G32_UINT;
  case VK_FORMAT_R32G32_SINT:
    return DXGI_FORMAT_R32G32_SINT;
  case VK_FORMAT_R32G32B32_SFLOAT:
    return DXGI_FORMAT_R32G32B32_FLOAT;
  case VK_FORMAT_R32G32B32_UINT:
    return DXGI_FORMAT_R32G32B32_UINT;
  case VK_FORMAT_R32G32B32_SINT:
    return DXGI_FORMAT_R32G32B32_SINT;
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case VK_FORMAT_R32G32B32A32_UINT:
    return DXGI_FORMAT_R32G32B32A32_UINT;
  case VK_FORMAT_R32G32B32A32_SINT:
    return DXGI_FORMAT_R32G32B32A32_SINT;
  }
  throw std::runtime_error("saveToDDS: unsupported src image format");
}

dds::ResourceDimension getResourceDimension(const Image& srcImage)
{
  if(srcImage.extent().z != 1) return dds::Texture3D;
  if(srcImage.extent().y != 1) return dds::Texture2D;
  return dds::Texture1D;
}

void mt::saveToDDS( const Image& srcImage,
                    const fs::path& file,
                    CommandQueueTransfer* transferQueue)
{
  Device& device = srcImage.device();
  if (transferQueue == nullptr) transferQueue = &device.primaryQueue();
  MT_ASSERT(&device == &transferQueue->device());

  std::vector<Ref<DataBuffer>> slicesBuffers = readSlices(srcImage,
                                                          device,
                                                          *transferQueue);

  VkFormat format = srcImage.format();
  const ImageFormatFeatures& formatDesc = getFormatFeatures(format);
  DXGI_FORMAT dxgiFormat = getDXGIFormat(formatDesc);

  std::ofstream fileStraem(file, std::ios::binary);
  if (!fileStraem.is_open()) throw std::runtime_error(std::string("Unable to open file: ") + (const char*)file.u8string().c_str());

  uint32_t ddsMagic = dds::DDS;
  fileStraem.write((const char*)&ddsMagic, sizeof(ddsMagic));

  uint32_t pixelFormatFlags = (uint32_t)dds::PixelFormatFlags::FourCC;
  if (formatDesc.hasA)
  {
    pixelFormatFlags |= (uint32_t)dds::PixelFormatFlags::AlphaPixels;
  }

  dds::FileHeader header{};
  header.pixelFormat.size = sizeof(header.pixelFormat);
  header.pixelFormat.flags = (dds::PixelFormatFlags)pixelFormatFlags;
  header.pixelFormat.fourCC = dds::DX10;
  header.size = sizeof(dds::FileHeader);
  header.mipmapCount = srcImage.mipmapCount();
  header.width = srcImage.extent().x;
  header.height = srcImage.extent().y;
  header.depth = srcImage.extent().z;

  fileStraem.write((const char*)&header, sizeof(header));

  dds::Dx10Header dx10Header{};
  dx10Header.dxgiFormat = dxgiFormat;
  dx10Header.resourceDimension = getResourceDimension(srcImage);
  dx10Header.arraySize = srcImage.arraySize();

  fileStraem.write((const char*)&dx10Header, sizeof(dx10Header));

  for(Ref<DataBuffer> buffer : slicesBuffers)
  {
    DataBuffer::Mapper map(*buffer, DataBuffer::Mapper::GPU_TO_CPU);
    fileStraem.write((const char*)map.data(), buffer->size());
  }
}
