#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <util/util.h>
#include <vkr/FrameBuffer.h>
#include <vkr/SwapChain.h>
#include <vkr/VKRLib.h>
#include <vkr/Win32WindowSurface.h>
#include <dumpHardware.h>

#include <vkr/pipeline/ShaderModule.h>

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

    Win32WindowSurface surface(glfwGetWin32Window(window));

    /*dumpHardware( false,      // dumpLayers
                  false,      // dumpExtensions
                  true,       // dumpDevices
                  true,       // dumpQueues
                  false,      // dumpMemory
                  &surface);*/

    std::unique_ptr<Device> device = vkrLib.createDevice(
                                                        {},
                                                        {},
                                                        GRAPHICS_CONFIGURATION,
                                                        &surface);

    Ref<SwapChain> swapChain(new SwapChain( *device,
                                            surface,
                                            std::nullopt,
                                            std::nullopt));

    {
      ShaderModule vertShader(*device, "shader.vert.spv");
      ShaderModule fragShader(*device, "shader.frag.spv");
    }

    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();

      SwapChain::FrameAccess frame(*swapChain);
      MT_ASSERT(frame.image() != nullptr);

      Ref<ImageView> colorTarget(new ImageView( *frame.image(),
                                                ImageSlice(*frame.image()),
                                                VK_IMAGE_VIEW_TYPE_2D));

      FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = colorTarget.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{.1f, .05f, .05f, 1.0f}};
      Ref<FrameBuffer> frameBuffer(
                            new FrameBuffer(
                                  std::span(&colorAttachment, 1),
                                  nullptr));

      std::unique_ptr<CommandProducerGraphic> producer = device->graphicQueue()->startCommands();

      producer->imageBarrier( *frame.image(),
                              ImageSlice(*frame.image()),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_ACCESS_NONE,
                              VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

      CommandProducerGraphic::RenderPass renderPass(*producer, *frameBuffer);
      renderPass.endPass();

      producer->imageBarrier( *frame.image(),
                              ImageSlice(*frame.image()),
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                              VK_ACCESS_NONE);

      device->graphicQueue()->submitCommands(std::move(producer));

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
