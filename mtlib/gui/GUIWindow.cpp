#include <stdexcept>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <gui/GUIWindow.h>
#include <gui/modalDialogs.h>
#include <gui/WindowConfiguration.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <vkr/VKRLib.h>

using namespace mt;

const GUIWindow* GUIWindow::_currentWindow = nullptr;

//  Стэк контекстов ImGUI. Нужен для корректного возврата к предыдущему
//  контексту, если потребовалось переключиться на контекст какого-либо окна
struct ImGuiContextInfo
{
  ImGuiContext* context;
  const GUIWindow* window;
};
static std::vector<ImGuiContextInfo> contextStack;

class GUIWindow::ImguiContextSetter
{
public:
  ImguiContextSetter(ImGuiContext& context, const GUIWindow& window)
  {
    contextStack.push_back(ImGuiContextInfo{&context, &window});
    ImGui::SetCurrentContext(&context);
    GUIWindow::_currentWindow = &window;
  }

  ~ImguiContextSetter()
  {
    MT_ASSERT(!contextStack.empty());
    contextStack.pop_back();

    ImGuiContext* previousContext = nullptr;
    const GUIWindow* previousWindow = nullptr;
    if(!contextStack.empty())
    {
      previousContext = contextStack.back().context;
      previousWindow = contextStack.back().window;
    }
    ImGui::SetCurrentContext(previousContext);
    GUIWindow::_currentWindow = previousWindow;
  }
};

GUIWindow::GUIWindow( Device& device,
                      const char* name,
                      std::optional<VkPresentModeKHR> presentationMode,
                      std::optional<VkSurfaceFormatKHR> swapchainFormat,
                      VkFormat bepthBufferFormat) :
  RenderWindow( device,
                name,
                presentationMode,
                swapchainFormat,
                bepthBufferFormat),
  _imguiContext(nullptr)
{
  _imguiContext = ImGui::CreateContext();
  ImguiContextSetter setter(*_imguiContext, *this);

  ImGuiIO& imGuiIO = ImGui::GetIO();
  imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imGuiIO.IniFilename = nullptr;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(&handle(), true);

  _initImGuiVulkanBackend();
}

void GUIWindow::_initImGuiVulkanBackend()
{
  VKRLib& vkrLib = VKRLib::instance();

  ImGui_ImplVulkan_InitInfo vulkanInitInfo{};
  vulkanInitInfo.ApiVersion = vkrLib.vulkanApiVersion();
  vulkanInitInfo.Instance = vkrLib.handle();
  vulkanInitInfo.PhysicalDevice = device().physicalDevice().handle();
  vulkanInitInfo.Device = device().handle();
  vulkanInitInfo.QueueFamily = device().graphicQueue()->family().index();
  vulkanInitInfo.Queue = device().graphicQueue()->handle();
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

  VkFormat depthFormat = depthBufferFormat();
  if(depthFormat != VK_FORMAT_UNDEFINED)
  {
    const ImageFormatFeatures& formatProps = getFormatFeatures(depthFormat);
    if(formatProps.hasDepth)
    {
      pipelineInfo.depthAttachmentFormat = depthFormat;
    }
    if(formatProps.hasStencil)
    {
      pipelineInfo.stencilAttachmentFormat = depthFormat;
    }
  }

  ImGui_ImplVulkan_Init(&vulkanInitInfo);
}

GUIWindow::~GUIWindow() noexcept
{
  _clean();
}

void GUIWindow::_clean() noexcept
{
  try
  {
    if (_imguiContext != nullptr)
    {
      device().presentationQueue()->waitIdle();

      ImguiContextSetter setter(*_imguiContext, *this);
      ImGui_ImplGlfw_Shutdown();
      ImGui_ImplVulkan_Shutdown();

      ImGui::DestroyContext(_imguiContext);

      _imguiContext = nullptr;
    }
  }
  catch(std::exception& error)
  {
    Log::error() << "GUIWindow::_clean" << error.what();
    Abort(error.what());
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

  ImguiContextSetter setter(*_imguiContext, *this);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  try
  {
    guiImplementation();
  }
  catch(std::exception& error)
  {
    Log::error() << error.what();
    errorDialog(this, "Error", error.what());
  }
}

void GUIWindow::guiImplementation()
{
}

void GUIWindow::drawImplementation(FrameBuffer& frameBuffer)
{
  std::unique_ptr<CommandProducerGraphic> commandProducer =
                                      device().graphicQueue()->startCommands();

  CommandProducerGraphic::RenderPass renderPass(*commandProducer, frameBuffer);
  drawGUI(*commandProducer);
  renderPass.endPass();

  device().graphicQueue()->submitCommands(std::move(commandProducer));
}

void GUIWindow::drawGUI(CommandProducerGraphic& commandProducer)
{
  commandProducer.beginDebugLabel("ImGui");

  ImguiContextSetter setter(*_imguiContext, *this);

  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  CommandBuffer& commandBuffer = commandProducer.getOrCreateBuffer();

  device().graphicQueue()->runSafe(
    [&]()
    {
      ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer.handle());
    });

  commandProducer.endDebugLabel();
}

void GUIWindow::applyConfiguration(const WindowConfiguration& configuration)
{
  RenderWindow::applyConfiguration(configuration);

  if(!configuration.imguiConfig.empty())
  {
    ImguiContextSetter setter(*_imguiContext, *this);
    ImGui::LoadIniSettingsFromMemory( configuration.imguiConfig.c_str(),
                                      configuration.imguiConfig.size());
  }
}

void GUIWindow::fillConfiguration(WindowConfiguration& configuration) const
{
  RenderWindow::fillConfiguration(configuration);

  ImguiContextSetter setter(*_imguiContext, *this);
  configuration.imguiConfig = ImGui::SaveIniSettingsToMemory();
  ImGuiIO& imGuiIO = ImGui::GetIO();
  imGuiIO.WantSaveIniSettings = false;
}
