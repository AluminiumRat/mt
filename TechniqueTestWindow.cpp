#include <vkr/queue/CommandProducerGraphic.h>
#include <TechniqueTestWindow.h>

using namespace mt;

TechniqueTestWindow::TechniqueTestWindow(Device& device) :
  GLFWRenderWindow(device, "Technique test"),
  _technique(new Technique(device, "Test technique")),
  _selector1(_technique->getOrCreateSelection("selector1")),
  _selector2(_technique->getOrCreateSelection("selector2"))
{
  _selector1.setValue("1");
  _selector2.setValue("1");

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

  if(frameIndex % 250 == 12)
  {
    _technique->configurator().rebuildConfiguration();
  }

  _technique->bindGraphic(commandProducer);
  commandProducer.draw(3);
  _technique->unbindGraphic(commandProducer);

  renderPass.endPass();
}
