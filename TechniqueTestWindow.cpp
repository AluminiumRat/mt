#include <TechniqueTestWindow.h>

using namespace mt;

TechniqueTestWindow::TechniqueTestWindow(Device& device) :
  GLFWRenderWindow(device, "Technique test"),
  _technique(new Technique(device))
{
  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);
  _technique->setFrameBufferFormat(&fbFormat);

  Technique::ShaderInfo shaders[2] =
  { {.stage = VK_SHADER_STAGE_VERTEX_BIT, .filename = "shader.vert.spv"},
    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .filename = "shader.frag.spv"}};
  _technique->setShaders(shaders);
  _technique->forceUpdate();
}