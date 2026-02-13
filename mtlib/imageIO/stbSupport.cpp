#include <stdexcept>

#include <imageIO/imageIO.h>
#include <util/ContentLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/Device.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace mt;

static Ref<Image> createImage(int width,
                              int height,
                              VkFormat format,
                              const void* data,
                              CommandQueueGraphic& transferQueue,
                              bool layoutAutocontrol,
                              const std::string& filename)
{
  if(width <= 0 || height <= 0) throw std::runtime_error(filename + " wrong image data");

  //  Создаем сам image
  glm::uvec2 extent(width, height);
  uint32_t mipsNumber = Image::calculateMipNumber(extent);
  Ref<Image> image(new Image( transferQueue.device(),
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                              0,
                              format,
                              glm::uvec3(extent, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              mipsNumber,
                              layoutAutocontrol,
                              filename.c_str()));

  std::unique_ptr<CommandProducerGraphic> producer =
                               transferQueue.startCommands("Image uploading");

  if(!layoutAutocontrol)
  {
    producer->imageBarrier( *image,
                            ImageSlice(*image),
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            0,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0);
  }

  //  Копируем нулевой мир
  const ImageFormatFeatures& formatInfo = getFormatFeatures(format);
  size_t dataSize = width * height * formatInfo.texelSize / 8;
  Ref<DataBuffer> uploadBuffer(new DataBuffer(transferQueue.device(),
                                              dataSize,
                                              DataBuffer::UPLOADING_BUFFER,
                                              "Image uploading buffer"));
  uploadBuffer->uploadData( data, 0, dataSize);
  producer->copyFromBufferToImage(*uploadBuffer,
                                  0,
                                  0,
                                  0,
                                  *image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  0,
                                  1,
                                  0,
                                  glm::uvec3(0,0,0),
                                  glm::uvec3(extent, 1));

  //  Генерируем мипы
  for(uint32_t srcMip = 0; srcMip < mipsNumber - 1; srcMip++)
  {
    if(!layoutAutocontrol)
    {
      producer->imageBarrier( *image,
                              ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                          srcMip,
                                          1,
                                          0,
                                          1),
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_TRANSFER_READ_BIT);
    }
    glm::uvec3 srcSize = image->extent(srcMip);
    glm::uvec3 dstSize = image->extent(srcMip + 1);
    producer->blitImage(*image,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        0,
                        srcMip,
                        glm::uvec3(0),
                        srcSize,
                        *image,
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        0,
                        srcMip + 1,
                        glm::uvec3(0),
                        dstSize,
                        VK_FILTER_LINEAR);
  }

  //  Переводим image в лэйаут VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  if(!layoutAutocontrol)
  {
    if(image->mipmapCount() > 1)
    {
      //  Все мипы кроме последнего находятся в
      //  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
      producer->imageBarrier( *image,
                              ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                          0,
                                          image->mipmapCount() - 1,
                                          0,
                                          1),
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              0,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              0,
                              VK_ACCESS_SHADER_READ_BIT);
    }

    //  Последний мип в VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    producer->imageBarrier( *image,
                            ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                        image->mipmapCount() - 1,
                                        1,
                                        0,
                                        1),
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_SHADER_READ_BIT);
  }

  transferQueue.submitCommands(std::move(producer));
  return image;
}

Ref<Image> mt::loadStbLDR(const std::filesystem::path& file,
                          CommandQueueGraphic& uploadQueue,
                          bool layoutAutocontrol)
{
  std::string filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& loader = ContentLoader::getLoader();
  std::vector<char> fileData = loader.loadData(file);

  //  Парсим данные
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* imageData = stbi_load_from_memory( (stbi_uc*)fileData.data(),
                                              (int)fileData.size(),
                                              &width,
                                              &height,
                                              &channels,
                                              STBI_rgb_alpha);
  if(imageData == nullptr)
  {
    throw std::runtime_error(std::string("unable to load image from ") + filename);
  }

  //  Загружаем данные на GPU
  try
  {
    Ref<Image> result = createImage(width,
                                    height,
                                    VK_FORMAT_R8G8B8A8_SRGB,
                                    imageData,
                                    uploadQueue,
                                    layoutAutocontrol,
                                    filename);
    stbi_image_free(imageData);
    return result;
  }
  catch(...)
  {
    stbi_image_free(imageData);
    throw;
  }
}

Ref<Image> mt::loadStbHDR(const std::filesystem::path& file,
                          CommandQueueGraphic& uploadQueue,
                          bool layoutAutocontrol)
{
  std::string filename = (const char*)file.u8string().c_str();

  //  Загружаем данные с диска
  ContentLoader& loader = ContentLoader::getLoader();
  std::vector<char> fileData = loader.loadData(file);

  //  Парсим данные
  int width = 0;
  int height = 0;
  int channels = 0;
  float* imageData = stbi_loadf_from_memory((stbi_uc*)fileData.data(),
                                            (int)fileData.size(),
                                            &width,
                                            &height,
                                            &channels,
                                            STBI_rgb_alpha);
  if(imageData == nullptr)
  {
    throw std::runtime_error(std::string("unable to load image from ") + filename);
  }

  //  Загружаем данные на GPU
  try
  {
    Ref<Image> result = createImage(width,
                                    height,
                                    VK_FORMAT_R32G32B32A32_SFLOAT,
                                    imageData,
                                    uploadQueue,
                                    layoutAutocontrol,
                                    filename);
    stbi_image_free(imageData);
    return result;
  }
  catch(...)
  {
    stbi_image_free(imageData);
    throw;
  }
}