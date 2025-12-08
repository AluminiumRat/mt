#include <filesystem>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <gui/GUIWindow.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <vkr/VKRLib.h>

using namespace mt;

// Место, куда будем писать конфиги imgui
std::string imguiIni = (const char*)
            (std::filesystem::current_path() / "imgui.ini").u8string().c_str();

//  Стэк контекстов ImGUI. Нужен для корректного возврата к предыдущему
//  контексту, если потребовалось переключиться на контекст какого-либо окна
static std::vector<ImGuiContext*> contextStack;

class ImguiContextSetter
{
public:
  ImguiContextSetter(ImGuiContext& context):
    _imguiContext(nullptr)
  {
    contextStack.push_back(&context);
    ImGui::SetCurrentContext(&context);
    _imguiContext = &context;
  }

  ~ImguiContextSetter()
  {
    if(_imguiContext == nullptr) return;

    MT_ASSERT(!contextStack.empty());
    contextStack.pop_back();

    ImGuiContext* previousContext = nullptr;
    if(!contextStack.empty())
    {
      previousContext = contextStack.back();
    }
    ImGui::SetCurrentContext(previousContext);
  }

private:
  ImGuiContext* _imguiContext;
};

GUIWindow::GUIWindow(Device& device, const char* name) :
  RenderWindow(device, name),
  _imguiContext(nullptr)
{
  _imguiContext = ImGui::CreateContext();
  ImguiContextSetter setter(*_imguiContext);

  ImGuiIO& imGuiIO = ImGui::GetIO();
  imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imGuiIO.IniFilename = imguiIni.c_str();

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

GUIWindow::~GUIWindow() noexcept
{
  _clean();
}

void GUIWindow::_clean() noexcept
{
  if (_imguiContext != nullptr)
  {
    device().presentationQueue()->waitIdle();

    ImguiContextSetter setter(*_imguiContext);
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplVulkan_Shutdown();

    ImGui::DestroyContext(_imguiContext);

    _imguiContext = nullptr;
  }
}

void GUIWindow::onClose() noexcept
{
  _clean();
  RenderWindow::onClose();
}

void GUIWindow::update()
{
  RenderWindow::update();
  if(!isVisible()) return;

  ImguiContextSetter setter(*_imguiContext);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  guiImplementation();
}

void GUIWindow::guiImplementation()
{
}

void GUIWindow::drawImplementation( CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer)
{
  ImguiContextSetter setter(*_imguiContext);

  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);

  CommandBuffer& commandBuffer = commandProducer.getOrCreateBuffer();
  ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.handle());

  renderPass.endPass();
}
