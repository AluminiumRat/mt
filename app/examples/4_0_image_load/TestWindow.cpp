#include <memory>
#include <vulkan/vulkan.h>

#include <imageIO/imageIO.h>
#include <technique/TechniqueLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow( device,
                "Test window",
                std::nullopt,
                std::nullopt,
                VK_FORMAT_UNDEFINED),
  _configurator(new TechniqueConfigurator(device, "Test technique")),
  _technique(*_configurator),
  _pass(_technique.getOrCreatePass("RenderPass")),
  _vertexBuffer(_technique.getOrCreateResourceBinding("vertices")),
  _texture(_technique.getOrCreateResourceBinding("colorTexture"))
{
  _makeConfiguration();
  _createVertexBuffer();
  _createTexture();
}

void TestWindow::_makeConfiguration()
{
  loadConfigurator(*_configurator, "examples/dds_load/technique.tch");
  _configurator->rebuildConfiguration();
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
                                        DataBuffer::STORAGE_BUFFER,
                                        "Vertex buffer"));
  device().graphicQueue()->uploadToBuffer(*buffer,
                                          0,
                                          sizeof(vertices),
                                          vertices);
  _vertexBuffer.setBuffer(buffer);
}

void TestWindow::_createTexture()
{
  //Ref<Image> image = loadImage( "examples/image.dds",
  //Ref<Image> image = loadImage( "examples/imageRGB.png",
  //Ref<Image> image = loadImage( "examples/imageRGBA.png",
  Ref<Image> image = loadImage( "textureProcessor/environmentMap/sunny_rose_garden_1k.hdr",
                                *device().graphicQueue(),
                                true);

  Ref<ImageView> imageView(new ImageView( *image,
                                          ImageSlice(*image),
                                          VK_IMAGE_VIEW_TYPE_2D));
  _texture.setImage(imageView);
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                      device().graphicQueue()->startCommands();

  CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);

  Technique::BindGraphic bind(_technique, _pass, *commandProducer);
  if (bind.isValid())
  {
    commandProducer->draw(4);
    bind.release();
  }

  renderPass.endPass();

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}
