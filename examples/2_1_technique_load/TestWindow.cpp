#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow(device, "Test window"),
  _configurator(new TechniqueConfigurator(device, "Test technique")),
  _technique(new Technique(*_configurator)),
  _pass(_technique->getOrCreatePass("RenderPass")),
  _colorSelector(_technique->getOrCreateSelection("colorSelector")),
  _vertexBuffer(_technique->getOrCreateResourceBinding("vertices")),
  _texture(_technique->getOrCreateResourceBinding("colorTexture")),
  _color(_technique->getOrCreateUniform("colorData.color"))
{
  _makeConfiguration();
  _createVertexBuffer();
  _createTexture();
}

void TestWindow::_makeConfiguration()
{
  //  Техника, описанная одним файлом. Внутри описаны все настройки со
  //  всеми значениями
  loadConfigurator(*_configurator, "examples/technique/technique.tch");

  //  Техника, использующая ссылки на библиотечные настройки
  //loadConfigurator(*_configurator, "examples/technique/techniqueWithRefs.tch");

  _configurator->rebuildConfiguration();
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
  _vertexBuffer.setBuffer(buffer);
}

void TestWindow::_createTexture()
{
  Ref<Image> image(new Image( device(),
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                              0,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              glm::uvec3(2, 2, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              true,
                              "Test image"));

  uint32_t pixels[4] = { 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000 };
  device().graphicQueue()->uploadToImage( *image,
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          0,
                                          0,
                                          glm::uvec3(0,0,0),
                                          glm::uvec3(2,2,1),
                                          pixels);

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

  static int frameIndex = 0;

  //  Выбираем вариант рендера через селекшен
  if (frameIndex % 360 < 180)
  {
    _colorSelector.setValue("0");
  }
  else
  {
    _colorSelector.setValue("1");
  }

  //  Выставляем волатильную юниформ переменную цвета
  float colorFactor = (frameIndex % 100) / 100.0f;
  glm::vec4 colorValue(colorFactor, colorFactor, colorFactor, colorFactor);
  _color.setValue(colorValue);

  //  Бинд техники и отрисовка
  Technique::Bind bind(*_technique, _pass, commandProducer);
  if (bind.isValid())
  {
    commandProducer.draw(3);
    bind.release();
  }

  renderPass.endPass();
  frameIndex++;
}
