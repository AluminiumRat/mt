#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  GLFWRenderWindow(device, "Test window"),
  _technique(new Technique(device, "Test technique")),
  _pass(_technique->getOrCreatePass("RenderPass")),
  _colorSelector(_technique->getOrCreateSelection("colorSelector")),
  _vertexBuffer(_technique->getOrCreateResource("vertices")),
  _texture(_technique->getOrCreateResource("colorTexture")),
  _sampler(_technique->getOrCreateResource("samplerState")),
  _color(_technique->getOrCreateUniform("colorData.color"))
{
  //  Настраивать и пересобирать конфигурацию можно и до и после настройки
  //  ресурсов и юниформов. Эту строку можно перенести в конец конструктора
  //  и даже в цикл рендера, но не внутрь бинда техники и не во время
  //  использования волатильного контекста.
  _makeConfiguration();

  //  Селектор можно установить заранее, и не выставлять каждый раз в цикле
  //  рендера
  //_colorSelector.setValue("0");

  _createVertexBuffer();
  _createTexture();
  _sampler.setSampler(ConstRef(new Sampler(device)));

  //  Юниформы можно выствить один раз заранее и не выставлять каждый раз
  //  в цикле рендера. Даже если они волатильные
  //_color.setValue(glm::vec4(1, 1, 1, 1));

  //  Можно перенести создание конфигурации сюда
  //_makeConfiguration();
}

void TestWindow::_makeConfiguration()
{
  PassConfigurator::ShaderInfo shaders[2] =
  { { .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .filename = "examples/technique/shader.vert"},
    { .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .filename = "examples/technique/shader.frag"}};

  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);

  std::unique_ptr<PassConfigurator> pass(new PassConfigurator("RenderPass"));
  pass->setFrameBufferFormat(&fbFormat);
  pass->setShaders(shaders);
  std::string passSelections[] = {"colorSelector"};
  pass->setSelections(passSelections);
  pass->setShaders(shaders);

  TechniqueConfigurator& configurator = _technique->configurator();
  configurator.addPass(std::move(pass));

  TechniqueConfigurator::SelectionDefine selections[] =
    { { .name = "colorSelector",
        .valueVariants = {"0", "1"}}};
  configurator.setSelections(selections);

  configurator.rebuildConfiguration();
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
                                        DataBuffer::STORAGE_BUFFER));
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
                              true));

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

  //  Можно перенести создание конфигурации сюда, и оно будет работать,
  //   но очень медленно
  //_makeConfiguration();

  //  Можно выбрать один из вариантов рендера
  _drawSimple(commandProducer);
  //_drawVolatileContext(commandProducer);

  renderPass.endPass();
}

void TestWindow::_drawSimple(CommandProducerGraphic& commandProducer)
{
  //  Упрощенный вариант рендера, в котором значения для селекшенов и юниформов
  //    выставляются в сами эти объекты (_colorSelector.setValue и
  //    _color.setValue)
  //  Если не заморачиваться многопотоковым рендером или константным рендером,
  //    то такой подход - ок. Иначе надо использовать волатильный контекст (см.
  //    метод _drawVolatileContext)

  static int frameIndex = 0;

  // Выбираем вариант рендера через селекшен
  if(frameIndex % 360 < 180)
  {
    _colorSelector.setValue("0");
  }
  else
  {
    _colorSelector.setValue("1");
  }

  // Выставляем волатильную юниформ переменную цвета
  float colorFactor = (frameIndex % 100) / 100.0f;
  glm::vec4 colorValue(colorFactor, colorFactor, colorFactor, 1.0f);
  _color.setValue(colorValue);

  if(_technique->bindGraphic(commandProducer, _pass))
  {
    commandProducer.draw(3);
    _technique->unbindGraphic(commandProducer);
  }

  frameIndex++;
}

void TestWindow::_drawVolatileContext(
                                  CommandProducerGraphic& commandProducer) const
{
  //  Отрисовка через волатильный контекст. Позволяет не менять состояние
  //    техники и зависимых объектов во время отрисовки. Этот подход нужен
  //    только если нужен константный или многопоточный рендер.
  TechniqueVolatileContext volatileContext =
                            _technique->createVolatileContext(commandProducer);

  static int frameIndex = 0;

  // Выбираем вариант рендера через селекшен
  if(frameIndex % 360 < 180)
  {
    _colorSelector.setValue(volatileContext, "0");
  }
  else
  {
    _colorSelector.setValue(volatileContext, "1");
  }

  // Выставляем волатильную юниформ переменную цвета
  float colorFactor = (frameIndex % 100) / 100.0f;
  glm::vec4 colorValue(colorFactor, colorFactor, colorFactor, 1.0f);
  _color.setValue(volatileContext, colorValue);

  if(_technique->bindGraphic(commandProducer, _pass, &volatileContext))
  {
    commandProducer.draw(3);
    _technique->unbindGraphic(commandProducer);
  }

  frameIndex++;
}
