#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TechniqueTestWindow.h>

using namespace mt;

TechniqueTestWindow::TechniqueTestWindow(Device& device) :
  GLFWRenderWindow(device, "Technique test"),
  _technique(new Technique(device, "Test technique")),
  _selector1(_technique->getOrCreateSelection("selector1")),
  _selector2(_technique->getOrCreateSelection("selector2")),
  _vertexBuffer(_technique->getOrCreateResource("vertices")),
  _texture1(_technique->getOrCreateResource("colorTexture1")),
  _texture2(_technique->getOrCreateResource("colorTexture2")),
  _sampler(new Sampler(device)),
  _samplerResource(_technique->getOrCreateResource("samplerState"))
{
  _selector1.setValue("1");
  _selector2.setValue("1");

  _createVertexBuffers(device);
  _vertexBuffer.setBuffer(_vertexBuffers[0]);

  _createTextures(device);
  _texture1.setImages(_textures);
  _texture2.setImage(_textures[0]);

  _samplerResource.setSampler(_sampler);

  TechniqueConfigurator& configurator = _technique->configurator();

  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);
  configurator.setFrameBufferFormat(&fbFormat);

  TechniqueConfigurator::ShaderInfo shaders[2] =
  { {.stage = VK_SHADER_STAGE_VERTEX_BIT, .filename = "shader.vert"},
    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .filename = "shader.frag"}};
  configurator.setShaders(shaders);

  TechniqueConfigurator::SelectionDefine selections[2] =
    { {.name = "selector1", .valueVariants = {"0", "1", "2"}},
      {.name = "selector2", .valueVariants = {"0", "1"}}};
  configurator.setSelections(selections);

  configurator.rebuildConfiguration();
}

void TechniqueTestWindow::_createVertexBuffers(Device& device)
{
  for(int i = 0; i < 2; i++)
  {
    struct Vertex
    {
      alignas(16) glm::vec3 position;
      alignas(16) glm::vec4 color;
    };
    Vertex vertices[3] ={{.position = {0.0f, (i * 2 - 1) * -0.5f, 0.0f},
                          .color = {1, 0, 0, 1}},
                         {.position = {0.5f, (i * 2 - 1) * 0.5f, 0.0f},
                          .color = {0, 1, 0, 1}},
                         {.position = {-0.5f, (i * 2 - 1) * 0.5f, 0.0f},
                          .color = {0, 0, 1, 1}}};
    _vertexBuffers[i] = Ref(new DataBuffer( device,
                                            sizeof(vertices),
                                            DataBuffer::STORAGE_BUFFER));
    device.graphicQueue()->uploadToBuffer(*_vertexBuffers[i],
                                          0,
                                          sizeof(vertices),
                                          vertices);
  }
}

void TechniqueTestWindow::_createTextures(Device& device)
{
  Ref<Image> image(new Image( device,
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              0,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              glm::uvec3(2, 2, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              true));

  uint32_t pixels[4] = { 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000 };
  device.graphicQueue()->uploadToImage( *image,
                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                        0,
                                        0,
                                        glm::uvec3(0,0,0),
                                        glm::uvec3(2,2,1),
                                        pixels);

  _textures[0] = Ref(new ImageView( *image,
                                    ImageSlice(*image),
                                    VK_IMAGE_VIEW_TYPE_2D));
  _textures[1] = Ref(new ImageView( *image,
                                    ImageSlice(*image),
                                    VK_IMAGE_VIEW_TYPE_2D));
}

void TechniqueTestWindow::drawImplementation(
                                      CommandProducerGraphic& commandProducer,
                                      FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  static int frameIndex = 0;
  frameIndex++;
  if(frameIndex % 360 < 180) _selector2.setValue("0");
  else _selector2.setValue("1");

  if (frameIndex % 180 < 60) _selector1.setValue("0");
  else if(frameIndex % 180 < 120) _selector1.setValue("1");
  else _selector1.setValue("2");

  _vertexBuffer.setBuffer(_vertexBuffers[frameIndex % 399 / 200]);

  if(frameIndex % 250 == 12)
  {
    _technique->configurator().rebuildConfiguration();
  }

  _technique->bindGraphic(commandProducer);
  commandProducer.draw(3);
  _technique->unbindGraphic(commandProducer);

  renderPass.endPass();
}
