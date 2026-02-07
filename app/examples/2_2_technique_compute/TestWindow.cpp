#include <technique/TechniqueLoader.h>
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
  _computeTechnique{
    .configurator{new TechniqueConfigurator(device, "Compute technique")},
    .technique{*_computeTechnique.configurator},
    .pass{_computeTechnique.technique.getOrCreatePass("ComputePass")},
    .texture{_computeTechnique.technique.getOrCreateResourceBinding(
                                                            "storageTexture")}},
  _renderTechnique{ 
    .configurator{new TechniqueConfigurator(device, "Render technique")},
    .technique{*_renderTechnique.configurator},
    .pass{_renderTechnique.technique.getOrCreatePass("RenderPass")},
    .colorSelector{_renderTechnique.technique.getOrCreateSelection(
                                                              "colorSelector")},
    .vertexBuffer{_renderTechnique.technique.getOrCreateResourceBinding(
                                                                  "vertices")},
    .texture{_renderTechnique.technique.getOrCreateResourceBinding(
                                                              "colorTexture")},
    .color{_renderTechnique.technique.getOrCreateUniform("colorData.color")}}
{
  loadConfigurator( *_computeTechnique.configurator,
                    "examples/compute/compute.tch");
  _computeTechnique.configurator->rebuildConfiguration();

  loadConfigurator( *_renderTechnique.configurator,
                    "examples/technique/technique.tch");
  _renderTechnique.configurator->rebuildConfiguration();

  _renderTechnique.colorSelector.setValue("1");
  _renderTechnique.color.setValue(glm::vec4(1));

  _createVertexBuffer();
  _createTexture();
}

void TestWindow::_createVertexBuffer()
{
  struct Vertex
  {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec4 color;
  };
  Vertex vertices[3] =
                    { {.position = {0.0f, -0.5f, 0.0f}, .color = {1, 0, 0, 1}},
                      {.position = {0.5f, 0.5f, 0.0f}, .color = {0, 1, 0, 1}},
                      {.position = {-0.5f, 0.5f, 0.0f}, .color = {0, 0, 1, 1}}};
  Ref<DataBuffer> buffer(new DataBuffer(device(),
                                        sizeof(vertices),
                                        DataBuffer::STORAGE_BUFFER,
                                        "Vertex buffer"));
  device().graphicQueue()->uploadToBuffer(*buffer,
                                          0,
                                          sizeof(vertices),
                                          vertices);
  _renderTechnique.vertexBuffer.setBuffer(buffer);
}

void TestWindow::_createTexture()
{
  _renderedImage = new Image( device(),
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT,
                              0,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              glm::uvec3(256, 256, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              true,
                              "Test image");

  Ref<ImageView> imageView(new ImageView( *_renderedImage,
                                          ImageSlice(*_renderedImage),
                                          VK_IMAGE_VIEW_TYPE_2D));
  _renderTechnique.texture.setImage(imageView);
  _computeTechnique.texture.setImage(imageView);
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                      device().graphicQueue()->startCommands();

  //  Заполнение текстуры через компьют
  Technique::BindCompute bindCompute( _computeTechnique.technique,
                                      _computeTechnique.pass,
                                      *commandProducer);
  if (bindCompute.isValid())
  {
    commandProducer->dispatch(32, 32, 1);
    bindCompute.release();
  }

  //  Отрисовка
  CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);
    Technique::BindGraphic bindRender(_renderTechnique.technique,
                                      _renderTechnique.pass,
                                      *commandProducer);
    if (bindRender.isValid())
    {
      commandProducer->draw(3);
      bindRender.release();
    }
  renderPass.endPass();

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}
