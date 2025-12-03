#include <memory>
#include <vulkan/vulkan.h>

#include <ddsSupport/ddsSupport.h>
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
  Ref<Image> image = loadDDS( "C:/projects/images/image.dds",
                              device(),
                              nullptr,
                              true);

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
