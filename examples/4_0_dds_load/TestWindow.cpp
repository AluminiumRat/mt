#include <memory>
#include <vulkan/vulkan.h>
#include <dds.hpp>

#include <technique/TechniqueLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Test window"),
  _technique(new Technique(device, "Test technique")),
  _pass(_technique->getOrCreatePass("RenderPass")),
  _vertexBuffer(_technique->getOrCreateResource("vertices")),
  _texture(_technique->getOrCreateResource("colorTexture"))
{
  _makeConfiguration();
  _createVertexBuffer();
  _createTexture();
}

void TestWindow::_makeConfiguration()
{
  TechniqueConfigurator& configurator = _technique->configurator();
  loadConfigurator(configurator, "examples/dds_load/technique.tch");
  configurator.rebuildConfiguration();
}

void TestWindow::_createVertexBuffer()
{
  struct Vertex
  {
    alignas(16) glm::vec3 position;
  };
  Vertex vertices[4] =  { {.position = {-0.5f, -0.5f, 0.0f}},
                          {.position = {-0.5f, 0.5f, 0.0f}},
                          {.position = {0.5f, -0.5f, 0.0f}},
                          {.position = {0.5f, 0.5f, 0.0f}}};
  Ref<DataBuffer> buffer(new DataBuffer(device(),
                                        sizeof(vertices),
                                        DataBuffer::STORAGE_BUFFER));
  device().graphicQueue()->uploadToBuffer(*buffer,
                                          0,
                                          sizeof(vertices),
                                          vertices);
  _vertexBuffer.setBuffer(buffer);
}

void TestWindow::_createTexture()
{
  const char* filename = "C:/projects/images/image.dds";

  dds::Image ddsImage;
  if(dds::readFile(filename, &ddsImage) != dds::Success)
  {
    throw std::runtime_error(std::string("Unable to read ") + filename);
  }
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

  VkImageType imageType;
  switch (ddsImage.dimension) {
  case dds::Texture1D:
    imageType = VK_IMAGE_TYPE_1D;
    break;
  case dds::Texture2D:
    imageType = VK_IMAGE_TYPE_2D;
    break;
  case dds::Texture3D:
    imageType = VK_IMAGE_TYPE_3D;
    break;
  default:
    throw std::runtime_error(std::string("Unsupported dimension") + filename);
  }

  VkFormat imageFormat = dds::getVulkanFormat(ddsImage.format,
                                              ddsImage.supportsAlpha);
  ImageFormatFeatures formatDesc = getFormatFeatures(imageFormat);
  if(!formatDesc.isColor)
  {
    throw std::runtime_error(std::string("Only color formats are supported: ") + filename);
  }

  Ref<Image> image(new Image( device(),
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
                              true));

  std::unique_ptr<CommandProducerCompute> producer =
                                        device().primaryQueue().startCommands();
  for (uint32_t mipIndex = 0; mipIndex < ddsImage.numMips; mipIndex++)
  {
    size_t dataSize = ddsImage.mipmaps[mipIndex].size();
    Ref<DataBuffer> uploadBuffer(new DataBuffer(device(),
                                                dataSize,
                                                DataBuffer::UPLOADING_BUFFER));
    uploadBuffer->uploadData(ddsImage.mipmaps[mipIndex].data(), 0, dataSize);

    glm::uvec3 dstExtent = image->extent(mipIndex);

    producer->copyFromBufferToImage(*uploadBuffer,
                                    0,
                                    0,
                                    0,
                                    *image,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    ddsImage.arraySize,
                                    mipIndex,
                                    glm::uvec3(0,0,0),
                                    dstExtent);
  }
  device().primaryQueue().submitCommands(std::move(producer));

  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  _texture.setImage(imageView);
}

void TestWindow::drawImplementation(
                                      CommandProducerGraphic& commandProducer,
                                      FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  Technique::Bind bind(*_technique, _pass, commandProducer);
  if (bind.isValid())
  {
    commandProducer.draw(4);
    bind.release();
  }

  renderPass.endPass();
}
