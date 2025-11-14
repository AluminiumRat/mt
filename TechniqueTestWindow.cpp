#include <TechniqueTestWindow.h>

using namespace mt;

TechniqueTestWindow::TechniqueTestWindow(Device& device) :
  GLFWRenderWindow(device, "Technique test"),
  _technique(new Technique(device))
{
  Technique::ShaderInfo shaders[2] =
  { {.stage = VK_SHADER_STAGE_VERTEX_BIT, .filename = "shader.vert.spv"},
    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .filename = "shader.frag.spv"}};
  _technique->setShaders(shaders);
  _technique->forceUpdate();
}