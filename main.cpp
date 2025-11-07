#include <exception>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <util/util.h>
#include <vkr/VKRLib.h>
#include <dumpHardware.h>
#include <GLFWRenderWindow.h>

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

  Ref<PipelineLayout> pipelineLayout(new PipelineLayout(device, {}));

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

    std::optional<GLFWRenderWindow> mainWindow;
    mainWindow.emplace(*device, "Vulkan window");

    while (!mainWindow->shouldClose())
    {
      glfwPollEvents();

      mainWindow->draw();
      //producer->setGraphicPipeline(*pipeline);
      //producer->draw(3);
      //renderPass.endPass();
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
