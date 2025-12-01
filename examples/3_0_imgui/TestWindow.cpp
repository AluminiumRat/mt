#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <vkr/VKRLib.h>
#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  GLFWRenderWindow(device, "Test window"),
  _imguiContext(nullptr),
  _imGuiIO(nullptr)
{
  _imguiContext = ImGui::CreateContext();

  _imGuiIO = &ImGui::GetIO();
  _imGuiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(&handle(), true);

  VKRLib& vkrLib = VKRLib::instance();

  ImGui_ImplVulkan_InitInfo vulkanInitInfo{};
  vulkanInitInfo.ApiVersion = vkrLib.vulkanApiVersion();
  vulkanInitInfo.Instance = vkrLib.handle();
  vulkanInitInfo.PhysicalDevice = device.physicalDevice().handle();
  vulkanInitInfo.Device = device.handle();
  vulkanInitInfo.QueueFamily = device.primaryQueue().family().index();
  vulkanInitInfo.Queue = device.primaryQueue().handle();
  vulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
  vulkanInitInfo.DescriptorPoolSize =
                              IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
  vulkanInitInfo.MinImageCount = swapChain().framesCount();
  vulkanInitInfo.ImageCount = swapChain().framesCount();

  vulkanInitInfo.MinAllocationSize = 1024 * 1024;

  vulkanInitInfo.UseDynamicRendering = true;
  vulkanInitInfo.PipelineInfoMain = ImGui_ImplVulkan_PipelineInfo{};
  VkPipelineRenderingCreateInfoKHR& pipelineInfo =
                    vulkanInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo;
  pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineInfo.colorAttachmentCount = 1;
  VkFormat targetFormat = swapChain().imageFormat().format;
  pipelineInfo.pColorAttachmentFormats = &targetFormat;

  ImGui_ImplVulkan_Init(&vulkanInitInfo);
}

TestWindow::~TestWindow() noexcept
{
  if(_imguiContext != nullptr)
  {
    device().presentationQueue()->waitIdle();

    ImGui::SetCurrentContext(_imguiContext);
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplVulkan_Shutdown();

    ImGui::DestroyContext(_imguiContext);
  }
}

void TestWindow::update()
{
  ImGui::SetCurrentContext(_imguiContext);

  if(size().x == 0 || size().y == 0) return;

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Hello, world!");
  ImGui::Text("This is some useful text.");
  ImGui::End();
}

void TestWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  ImGui::SetCurrentContext(_imguiContext);

  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  CommandBuffer& commandBuffer = commandProducer.getOrCreateBuffer();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.handle());

  renderPass.endPass();
}
