#include <stdexcept>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <gui/BaseWindow.h>
#include <gui/GUILib.h>

using namespace mt;

GUILib* GUILib::_instance = nullptr;

GUILib::GUILib(const char* configFilename) :
  _configFilename(configFilename)
{
  MT_ASSERT(_instance == nullptr);
  if(glfwInit() == GLFW_FALSE) throw std::runtime_error("GUILib: unable to init GLFW");
  _instance = this;
}

GUILib::~GUILib() noexcept
{
  MT_ASSERT(_instance != nullptr);
  MT_ASSERT(_windows.empty());

  glfwTerminate();

  _instance = nullptr;
}

void GUILib::updateWindows()
{
  glfwPollEvents();
  for(BaseWindow* window : _windows) window->update();
}

void GUILib::drawWindows()
{
  for (BaseWindow* window : _windows) window->draw();
}
