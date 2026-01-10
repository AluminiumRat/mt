#include <stdexcept>

#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>

#include <gui/RenderWindow.h>
#include <vkr/Device.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vkr/Win32WindowSurface.h>

using namespace mt;

RenderWindow::RenderWindow(Device& device, const char* name) :
  BaseWindow(name),
  _device(device)
{
  try
  {
    _surface.reset(new Win32WindowSurface((HWND)platformDescriptor()));
    MT_ASSERT(device.isSurfaceSuitable(*_surface));
    _createSwapchain();
  }
  catch(...)
  {
    cleanup();
    throw;
  }
}

void RenderWindow::_createSwapchain()
{
  _deleteSwapchain();
  if(size().x == 0 || size().y == 0) return;

  _swapChain = Ref<SwapChain>(
                new SwapChain(_device, *_surface, std::nullopt, std::nullopt));
}

RenderWindow::~RenderWindow() noexcept
{
  cleanup();
}

void RenderWindow::cleanup() noexcept
{
  _deleteSwapchain();
  BaseWindow::cleanup();
}

void RenderWindow::_deleteSwapchain() noexcept
{
  std::unique_lock queuesLock = _device.lockQueues();
  _frameBuffers.clear();
  _swapChain.reset();
}

void RenderWindow::draw()
{
  if (!isVisible()) return;
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

Ref<FrameBuffer> RenderWindow::createFrameBuffer(Image& targetColorBuffer)
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

void RenderWindow::drawImplementation(CommandProducerGraphic& commandProducer,
                                      FrameBuffer& frameBuffer)
{
  CommandProducerGraphic::RenderPass renderPass(commandProducer, frameBuffer);
  renderPass.endPass();
}

void RenderWindow::onResize() noexcept
{
  BaseWindow::onResize();

  _deleteSwapchain();

  try
  {
    _createSwapchain();
  }
  catch (std::exception& error)
  {
    Log::error() << "RenderWindow::onResize: " << error.what();
    Abort("Unable to create new swaochain");
  }
}

void RenderWindow::onClose() noexcept
{
  _deleteSwapchain();
  BaseWindow::onClose();
}
