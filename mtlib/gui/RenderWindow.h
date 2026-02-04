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
    //  presentationMode - режим презентации свапчейна. Если nullopt, то режим
    //    будет выбран автоматически
    //  swapchainFormat - формат image в свапчейне. Если nullopt, то формат будет
    //    выбран автоматически
    //  bepthBufferFormat - формат деф буфера. Если VK_FORMAT_UNDEFINED, то деф
    //    буфер не будет использоваться
    RenderWindow( Device& device,
                  const char* name,
                  std::optional<VkPresentModeKHR> presentationMode,
                  std::optional<VkSurfaceFormatKHR> swapchainFormat,
                  VkFormat bepthBufferFormat);
    RenderWindow(const RenderWindow&) = delete;
    RenderWindow& operator = (const RenderWindow&) = delete;
    virtual ~RenderWindow() noexcept;

    inline Device& device() const noexcept;
    inline VkPresentModeKHR presentationMode() const noexcept;
    inline VkSurfaceFormatKHR swapchainFormat() const noexcept;
    inline VkFormat depthBufferFormat() const noexcept;

    virtual void draw() override;

    //  Необходимо ли чистить фрэйм буффер перед отрисовкой
    inline bool needClearFrameBuffer() const noexcept;
    inline void setNeedClearFrameBuffer(bool newValue)  noexcept;

    //  Цвер, в который будет чиститься таргет цвета
    inline const glm::vec4& clearColor() const noexcept;
    inline void setClearColor(const glm::vec4& newValue) noexcept;

    //  Значение для очистки дэф буфера
    inline float clearDepth() const noexcept;
    inline void setClearDepth(float newValue) noexcept;

    //  Значение для очистки стэнсила
    inline uint32_t clearStencil() const noexcept;
    inline void setClearStencil(uint32_t newValue) noexcept;

  protected:
    inline const SwapChain& swapChain() const noexcept;

    void cleanup() noexcept;

  protected:
    //  Здесь должен находится сам код отрисовки
    //  Image из свапчейна находится в лэйауте
    //    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, этот же лэйаут должен
    //    быть выставлен по окончании метода. Об остальных Image из фрэйм
    //    буфера должен беспокоиться сам метод drawImplementation, гарантируется
    //    что лэйаут не будет измерен вне этого метода.
    virtual void drawImplementation(FrameBuffer& frameBuffer);

  protected:
    virtual void onResize() noexcept override;
    virtual void onClose() noexcept override;

  private:
    void _createSwapchain();
    Ref<FrameBuffer> _createFrameBuffer(Image& targetColorBuffer);
    void _deleteSwapchain() noexcept;

  private:
    Device& _device;
    std::unique_ptr<WindowSurface> _surface;

    std::optional<VkPresentModeKHR> _presentationMode;
    std::optional<VkSurfaceFormatKHR> _swapchainFormat;
    VkFormat _depthBufferFormat;

    Ref<SwapChain> _swapChain;
    std::vector<Ref<FrameBuffer>> _frameBuffers;
    Ref<ImageView> _depthBufferView;

    bool _needClearFrameBuffer;
    glm::vec4 _clearColor;
    float _clearDepth;
    uint32_t _clearStencil;
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

  inline VkSurfaceFormatKHR RenderWindow::swapchainFormat() const noexcept
  {
    MT_ASSERT(_swapchainFormat != std::nullopt);
    return *_swapchainFormat;
  }

  inline VkFormat RenderWindow::depthBufferFormat() const noexcept
  {
    return _depthBufferFormat;
  }

  inline bool RenderWindow::needClearFrameBuffer() const noexcept
  {
    return _needClearFrameBuffer;
  }
  
  inline void RenderWindow::setNeedClearFrameBuffer(bool newValue)  noexcept
  {
    if(_needClearFrameBuffer == newValue) return;
    _needClearFrameBuffer = newValue;
    _frameBuffers.clear();
  }

  inline const glm::vec4& RenderWindow::clearColor() const noexcept
  {
    return _clearColor;
  }
  
  inline void RenderWindow::setClearColor(const glm::vec4& newValue) noexcept
  {
    if(_clearColor == newValue) return;
    _clearColor = newValue;
    _frameBuffers.clear();
  }

  inline float RenderWindow::clearDepth() const noexcept
  {
    return _clearDepth;
  }
  
  inline void RenderWindow::setClearDepth(float newValue) noexcept
  {
    if(_clearDepth == newValue) return;
    _clearDepth = newValue;
    _frameBuffers.clear();
  }

  inline uint32_t RenderWindow::clearStencil() const noexcept
  {
    return _clearStencil;
  }
  
  inline void RenderWindow::setClearStencil(uint32_t newValue) noexcept
  {
    if(_clearStencil == newValue) return;
    _clearStencil = newValue;
    _frameBuffers.clear();
  }

  inline const SwapChain& RenderWindow::swapChain() const noexcept
  {
    MT_ASSERT(_swapChain != nullptr);
    return *_swapChain;
  }
}