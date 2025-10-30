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
                                                        {},
                                                        {},
                                                        GRAPHICS_CONFIGURATION,
                                                        &surface);

    mt::Ref<mt::SwapChain> swapChain(new mt::SwapChain( *device,
                                                        surface,
                                                        std::nullopt,
                                                        std::nullopt));

    mt::Ref<mt::PlainBuffer> uploadBuffer(
                                new mt::PlainBuffer(
                                            *device,
                                            2 * 2 * 4,
                                            mt::PlainBuffer::UPLOADING_BUFFER));
    std::vector<uint32_t> data = {0xFFFFFFFF, 0xFF0000FF, 0xFF00FF00, 0xFFFF0000};
    uploadBuffer->uploadData(data.data(), 0, data.size() * 4);

    mt::Ref<mt::Image> image(new mt::Image( *device,
                                            VK_IMAGE_TYPE_2D,
                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                              VK_IMAGE_USAGE_SAMPLED_BIT,
                                            0,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            glm::uvec3(4,4,1),
                                            VK_SAMPLE_COUNT_1_BIT,
                                            1,
                                            2,
                                            true));

    std::unique_ptr<mt::CommandProducerTransfer> producer = device->transferQueue().startCommands();
    producer->copyFromBufferToImage(*uploadBuffer,
                                    0,
                                    2,
                                    2,
                                    *image,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    1,
                                    glm::uvec3(0,0,0),
                                    glm::uvec3(2,2,1));
    device->transferQueue().submitCommands(std::move(producer));
    uploadBuffer.reset();

    CommandQueue::ownershipTransfer(device->transferQueue(),
                                    *device->graphicQueue(),
                                    *image);

    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();

      mt::SwapChain::FrameAccess frame(*swapChain);
      MT_ASSERT(frame.image() != nullptr);

      std::unique_ptr<mt::CommandProducerGraphic> producer = device->graphicQueue()->startCommands();

      producer->blitImage(*image,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0,
                          1,
                          glm::uvec3(0,0,0),
                          glm::uvec3(2,2,1),
                          *image,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0,
                          0,
                          glm::uvec3(0,0,0),
                          glm::uvec3(4,4,1),
                          VK_FILTER_LINEAR);

      producer->imageBarrier( *frame.image(),
                              ImageSlice(*frame.image()),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_ACCESS_NONE,
                              VK_ACCESS_NONE);

      producer->blitImage(*image,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0,
                          0,
                          glm::uvec3(0,0,0),
                          glm::uvec3(4,4,1),
                          *frame.image(),
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          0,
                          0,
                          glm::uvec3(39,39,0),
                          glm::uvec3(140,140,1),
                          VK_FILTER_LINEAR);

      producer->imageBarrier( *frame.image(),
                              ImageSlice(*frame.image()),
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_ACCESS_NONE);

      device->primaryQueue().submitCommands(std::move(producer));

      frame.present();
    }

    image.reset();

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
