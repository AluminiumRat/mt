#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <util/util.h>
#include <vkr/VKRLib.h>

#include <TestWindow.h>

using namespace mt;

int main(int argc, char* argv[])
{
  try
  {
    VKRLib vkrLib("VKTest",
                  VKRLib::AppVersion{.major = 0, .minor = 0, .patch = 0},
                  VK_API_VERSION_1_3,
                  true,
                  true);

    glfwInit();

    {
      std::unique_ptr<Device> device = GLFWRenderWindow::createDevice(
                                                        {},
                                                        {},
                                                        GRAPHICS_CONFIGURATION);
      TestWindow mainWindow(*device);
      while (!mainWindow.shouldClose())
      {
        glfwPollEvents();
        mainWindow.draw();
      }
    }

    glfwTerminate();
    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();

    glfwTerminate();
    return 1;
  }
}
