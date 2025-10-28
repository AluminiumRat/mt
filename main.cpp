#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <util/util.h>
#include <vkr/SwapChain.h>
#include <vkr/VKRLib.h>
#include <vkr/Win32WindowSurface.h>
#include <dumpHardware.h>

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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    mt::Win32WindowSurface surface(glfwGetWin32Window(window));

    /*dumpHardware( false,      // dumpLayers
                  false,      // dumpExtensions
                  true,       // dumpDevices
                  true,       // dumpQueues
                  false,      // dumpMemory
                  &surface);*/

    std::unique_ptr<mt::Device> device = vkrLib.createDevice(
                                            {}, {}, true, true, true, &surface);

    mt::Ref<mt::SwapChain> swapChain(new mt::SwapChain( *device,
                                                        surface,
                                                        std::nullopt,
                                                        std::nullopt));

    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();

      mt::SwapChain::FrameAccess frame(*swapChain);
      MT_ASSERT(frame.image() != nullptr);

      std::unique_ptr<mt::CommandProducer> producer = device->primaryQueue().startCommands();

      producer->imageBarrier( *frame.image(),
                              ImageSlice(*frame.image()),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_ACCESS_MEMORY_WRITE_BIT,
                              VK_ACCESS_NONE);

      device->primaryQueue().submitCommands(std::move(producer));

      frame.present();
    }

    swapChain.reset();
    device.reset();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
