#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <util/util.h>
#include <vkr/dumpHardware.h>
#include <vkr/VKRLib.h>
#include <TechniqueTestWindow.h>
//#include <TestWindow.h>

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

    /*dumpHardware( false,      // dumpLayers
                  false,      // dumpExtensions
                  true,       // dumpDevices
                  true,       // dumpQueues
                  false,      // dumpMemory
                  &surface);*/

    std::unique_ptr<Device> device = GLFWRenderWindow::createDevice(
                                                        {},
                                                        {},
                                                        GRAPHICS_CONFIGURATION);

    //std::optional<TestWindow> mainWindow;
    std::optional<TechniqueTestWindow> mainWindow;
    mainWindow.emplace(*device);

    while (!mainWindow->shouldClose())
    {
      glfwPollEvents();
      mainWindow->draw();
    }

    mainWindow.reset();
    device.reset();

    glfwTerminate();

    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
