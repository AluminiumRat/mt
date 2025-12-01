#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/queue/QueueSources.h>
#include <vkr/FrameBuffer.h>
#include <vkr/SwapChain.h>
#include <vkr/WindowSurface.h>

struct GLFWwindow;

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  // Базовый класс окна для рендера, построенный на GLFW.
  // Реализует рутины по управлению окном, свапчейном и циклом рендера. При этом
  //  сам рендер должен быть реализован потомками.
  // glfwInit должен быть вызван до созданиея первого окна
  class GLFWRenderWindow
  {
  public:
    GLFWRenderWindow(Device& device, const char* title);
    GLFWRenderWindow(const GLFWRenderWindow&) = delete;
    GLFWRenderWindow& operator = (const GLFWRenderWindow&) = delete;
    virtual ~GLFWRenderWindow() noexcept;

    void draw();

    // Размер активной области для рисования(колор таргета). То есть сюда не
    //  включены рамки, кнопки и тайтл бар.
    inline glm::vec2 size() const noexcept;

    // Пользователь закрыл окно
    bool shouldClose() const noexcept;

    // Создать устройство, подходящее для рендера в окно
    static std::unique_ptr<Device> createDevice(
                            VkPhysicalDeviceFeatures requiredFeatures,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration);

  protected:
    inline Device& device() const noexcept;
    inline GLFWwindow& handle() const noexcept;
    inline const SwapChain& swapChain() const noexcept;

    //  Обработчик изменения размеров. К моменту вызова этого обработчика
    //  все команды из очередей GPU будут выполнены, а сами очереди
    //  заблокированы от доступа из других потоков.
    virtual void onResize() noexcept;
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

  private:
    void _createSwapchain();
    void _cleanup() noexcept;
    void _deleteSwapchain() noexcept;
    static void _resizeHandler(GLFWwindow* window, int width, int height);

  private:
    Device& _device;
    GLFWwindow* _handle;
    std::unique_ptr<WindowSurface> _surface;
    Ref<SwapChain> _swapChain;
    std::vector<Ref<FrameBuffer>> _frameBuffers;

    glm::uvec2 _size;
  };

  inline glm::vec2 GLFWRenderWindow::size() const noexcept
  {
    return _size;
  }

  inline Device& GLFWRenderWindow::device() const noexcept
  {
    return _device;
  }

  inline GLFWwindow& GLFWRenderWindow::handle() const noexcept
  {
    return *_handle;
  }

  inline const SwapChain& GLFWRenderWindow::swapChain() const noexcept
  {
    MT_ASSERT(_swapChain != nullptr);
    return *_swapChain;
  }
}