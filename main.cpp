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
#include <vkr/pipeline/GraphicPipeline.h>

using namespace mt;

Ref<GraphicPipeline> makePipeline(Device& device)
{
  VkFormat colorAttachments[1] = { VK_FORMAT_B8G8R8A8_SRGB };
  FrameBufferFormat fbFormat(colorAttachments, VK_FORMAT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT);

  ShaderModule vertShader(device, "shader.vert.spv");
  ShaderModule fragShader(device, "shader.frag.spv");
  AbstractPipeline::ShaderInfo shadersInfo[2]
    { { .module = &vertShader,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .entryPoint = "main"},
      { .module = &fragShader,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .entryPoint = "main"}};

  VkPipelineRasterizationStateCreateInfo rasterizationState{};
  rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.lineWidth = 1;

  VkPipelineDepthStencilStateCreateInfo depthStencilState{};
  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  VkPipelineColorBlendStateCreateInfo blendingState{};
  blendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendingState.attachmentCount = 1;
  VkPipelineColorBlendAttachmentState attachmentState{};
  attachmentState.colorWriteMask =  VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT;
  blendingState.pAttachments = &attachmentState;

  Ref<PipelineLayout> pipelineLayout(new PipelineLayout(device));

  return Ref(new GraphicPipeline( device,
                                  fbFormat,
                                  shadersInfo,
                                  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                  rasterizationState,
                                  depthStencilState,
                                  blendingState,
                                  *pipelineLayout));
}

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

    Ref<GraphicPipeline> pipeline = makePipeline(*device);

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
      producer->setGraphicPipeline(*pipeline);
      producer->draw(3);
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

    pipeline.reset();
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
