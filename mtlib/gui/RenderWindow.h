#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <gui/BaseWindow.h>
#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/FrameBuffer.h>
#include <vkr/SwapChain.h>
#include <vkr/WindowSurface.h>

struct GLFWwindow;

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  //  Базовый класс окна для рендера.
  //  Реализует рутины по управлению окном, свапчейном и циклом рендера. При этом
  //    сам рендер должен быть реализован потомками.
  //  GUILib должна быть инициализированная до создания первого окна
  class RenderWindow : public BaseWindow
  {
  public:
    RenderWindow(
              Device& device,
              const char* name,
              std::optional<VkPresentModeKHR> presentationMode = std::nullopt,
              std::optional<VkSurfaceFormatKHR> format = std::nullopt);
    RenderWindow(const RenderWindow&) = delete;
    RenderWindow& operator = (const RenderWindow&) = delete;
    virtual ~RenderWindow() noexcept;

    inline Device& device() const noexcept;
    inline VkPresentModeKHR presentationMode() const noexcept;
    inline VkSurfaceFormatKHR format() const noexcept;

    virtual void draw() override;

  protected:
    inline const SwapChain& swapChain() const noexcept;

    void cleanup() noexcept;

  protected:
    //  Если нужно создать фрэйм буфер, состоящий не только из Image-а из
    //  свапчейна, то в потомке необходимо переопределить этот метод
    virtual Ref<FrameBuffer> createFrameBuffer(Image& targetColorBuffer);

    //  Здесь должен находится сам код отрисовки
    //  Image из свапчейна находится в лэйауте
    //    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, этот же лэйаут должен
    //    быть выставлен по окончании метода. Об остальных Image из фрэйм
    //    буфера должен беспокоиться сам метод drawImplementation, гарантируется
    //    что лэйаут не будет измерен вне этого метода.
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer);

  protected:
    virtual void onResize() noexcept override;
    virtual void onClose() noexcept override;

  private:
    void _createSwapchain();
    void _deleteSwapchain() noexcept;

  private:
    Device& _device;
    std::unique_ptr<WindowSurface> _surface;

    std::optional<VkPresentModeKHR> _presentationMode;
    std::optional<VkSurfaceFormatKHR> _format;

    Ref<SwapChain> _swapChain;
    std::vector<Ref<FrameBuffer>> _frameBuffers;
  };

  inline Device& RenderWindow::device() const noexcept
  {
    return _device;
  }

  inline VkPresentModeKHR RenderWindow::presentationMode() const noexcept
  {
    MT_ASSERT(_presentationMode != std::nullopt);
    return *_presentationMode;
  }

  inline VkSurfaceFormatKHR RenderWindow::format() const noexcept
  {
    MT_ASSERT(_presentationMode != std::nullopt);
    return *_format;
  }

  inline const SwapChain& RenderWindow::swapChain() const noexcept
  {
    MT_ASSERT(_swapChain != nullptr);
    return *_swapChain;
  }
}