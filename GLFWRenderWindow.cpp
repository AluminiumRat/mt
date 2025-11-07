#include <stdexcept>

#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>

#include <vkr/Device.h>
#include <vkr/VKRLib.h>
#include <GLFWRenderWindow.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vkr/Win32WindowSurface.h>

using namespace mt;

GLFWRenderWindow::GLFWRenderWindow(Device& device, const char* title) :
  _device(device),
  _handle(nullptr),
  _size(800,600)
{
  try
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _handle = glfwCreateWindow(_size.x, _size.y, title, nullptr, nullptr);
    glfwSetWindowUserPointer(_handle, this);
    glfwSetWindowSizeCallback(_handle, &GLFWRenderWindow::_resizeHandler);

    _surface.reset(new Win32WindowSurface(glfwGetWin32Window(_handle)));
    MT_ASSERT(device.isSurfaceSuitable(*_surface));
    _createSwapchain();
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

void GLFWRenderWindow::_createSwapchain()
{
  _deleteSwapchain();
  if(_size.x == 0 || _size.y == 0) return;

  _swapChain = Ref<SwapChain>(
                new SwapChain(_device, *_surface, std::nullopt, std::nullopt));
}

GLFWRenderWindow::~GLFWRenderWindow() noexcept
{
  _cleanup();
}

void GLFWRenderWindow::_cleanup() noexcept
{
  _deleteSwapchain();
  if(_handle != nullptr)
  {
    glfwDestroyWindow(_handle);
    _handle = nullptr;
  }
}

void GLFWRenderWindow::_deleteSwapchain() noexcept
{
  std::unique_lock queuesLock = _device.lockQueues();
  _frameBuffers.clear();
  _swapChain.reset();
}

void GLFWRenderWindow::draw()
{
  if (_size.x == 0 || _size.y == 0) return;
  MT_ASSERT(_swapChain != nullptr);
  MT_ASSERT(_swapChain->framesCount() != 0);

  //  Если это первая отрисовка или свапчейн был пересоздан, то надо создать
  //  фрэймбуферы.
  if(_frameBuffers.empty())
  {
    for(uint32_t frameIndex = 0;
        frameIndex < _swapChain->framesCount();
        frameIndex++)
    {
      Ref<FrameBuffer> newBuffer =
                              createFrameBuffer(_swapChain->frame(frameIndex));
      _frameBuffers.push_back(newBuffer);
    }
  }

  // Захватываем очередной кадр из свапчейна
  SwapChain::FrameAccess frame(*_swapChain);
  MT_ASSERT(frame.image() != nullptr);

  std::unique_ptr<CommandProducerGraphic> producer =
                                        _device.graphicQueue()->startCommands();
  producer->imageBarrier( *frame.image(),
                          ImageSlice(*frame.image()),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_ACCESS_NONE,
                          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

  drawImplementation(*producer, *_frameBuffers[frame.frameIndex()]);

  producer->imageBarrier( *frame.image(),
                          ImageSlice(*frame.image()),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                          VK_ACCESS_NONE);
  _device.graphicQueue()->submitCommands(std::move(producer));
  frame.present();
}

Ref<FrameBuffer> GLFWRenderWindow::createFrameBuffer(Image& targetColorBuffer)
{
  Ref<ImageView> colorTarget(new ImageView( targetColorBuffer,
                                            ImageSlice(targetColorBuffer),
                                            VK_IMAGE_VIEW_TYPE_2D));

  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = colorTarget.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.05f, 0.1f, 0.05f, 1.0f}};

  return Ref<FrameBuffer>(new FrameBuffer(
                                std::span(&colorAttachment, 1),
                                nullptr));
}

void GLFWRenderWindow::drawImplementation(
                                        CommandProducerGraphic& commandProducer,
                                        FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);
  renderPass.endPass();
}

bool GLFWRenderWindow::shouldClose() const noexcept
{
  return glfwWindowShouldClose(_handle);
}

void GLFWRenderWindow::_resizeHandler(GLFWwindow* window, int width, int height)
{
  GLFWRenderWindow* renderWindow =
                          (GLFWRenderWindow*)(glfwGetWindowUserPointer(window));
  MT_ASSERT(renderWindow != nullptr);

  renderWindow->_size = glm::uvec2(width, height);
  renderWindow->onResize();
}

void GLFWRenderWindow::onResize() noexcept
{
  _deleteSwapchain();

  try
  {
    _createSwapchain();
  }
  catch (std::exception& error)
  {
    Log::error() << "GLFWRenderWindow::onResize: " << error.what();
    Abort("Unable to create new swaochain");
  }
}

std::unique_ptr<Device> GLFWRenderWindow::createDevice(
                            VkPhysicalDeviceFeatures requiredFeatures,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* testWindow = glfwCreateWindow(1, 1, "Test window", nullptr, nullptr);

  std::unique_ptr<Device> device;
  try
  {
    Win32WindowSurface testSurface(glfwGetWin32Window(testWindow));
    device = VKRLib::instance().createDevice( requiredFeatures,
                                              requiredExtensions,
                                              configuration,
                                              &testSurface);
  }
  catch(...)
  {
    glfwDestroyWindow(testWindow);
    throw;
  }

  glfwDestroyWindow(testWindow);
  return device;
}
