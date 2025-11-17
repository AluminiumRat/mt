#include <TechniqueTestWindow.h>

using namespace mt;

TechniqueTestWindow::TechniqueTestWindow(Device& device) :
  GLFWRenderWindow(device, "Technique test"),
  _configurator(new TechniqueConfigurator(device, "Test technique"))
{
  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);
  _configurator->setFrameBufferFormat(&fbFormat);

  TechniqueConfigurator::ShaderInfo shaders[2] =
  { {.stage = VK_SHADER_STAGE_VERTEX_BIT, .filename = "shader.vert"},
    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .filename = "shader.frag"}};
  _configurator->setShaders(shaders);

  TechniqueConfigurator::SelectionDefine selections[2] =
    { {.name = "selector1", .valueVariants = {"0", "1", "2"}},
      {.name = "selector2", .valueVariants = {"0", "1"}}};
  _configurator->setSelections(selections);

  _configurator->rebuildConfiguration();
}