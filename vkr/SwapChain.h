#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/queue/Fence.h>
#include <vkr/queue/Semaphore.h>

namespace mt
{
  class Device;
  class WindowSurface;
  class CommandQueue;

  //  Обертка вокруг вулкановского свапчейна
  //  Куча рутин по презентации отрендеренной картинки в оконной системе.
  //  Для того чтобы отправить изображение на презентацию, необходимо получить
  //    очередной кадр от свапчейна. Для этого используется вспомогательный класс
  //    FrameAccess, позволяющий безопасно захватывать и возвращать кадры
  //    свапчейна.
  //  Одновременно может быть захвачено не более 1 кадра
  //  Освобождение и презентация происходит через FrameAccess::present
  //  Свапчейн не позволяет бесконечно набивать очередь команд, при возвращении
  //    кадра в свапчейн происходит синхронизация GPU и CPU, гарантирующая
  //    что все команды для этого изображения с предыдущих кадров выполнены. То
  //    есть свапчейн может использоваться для синхронизации CPU-GPU в основном
  //    цикле.
  class SwapChain : public RefCounter
  {
  public:
    //  Вспомогательный класс для безопасного извлечения очередного кадра из
    //  свапчейна и возвращения его обратно в свапчейн
    //  В нормальных условиях необходимо каждый кадр создавать новый FrameAcces
    //    далее из него получить Image, скопировать в него полученную
    //    картинку и вызвать present
    //  Класс защищает от прерываний во время рендера. Деструктор FrameAccess
    //    аварийно вернет кадр в свапчейн, но при этом не произойдет
    //    презентация и не произойдет синхронизация с основной очередью команд.
    struct FrameAccess
    {
    public:
      inline FrameAccess(SwapChain& swapChain);
      FrameAccess(const FrameAccess&) = delete;
      FrameAccess& operator = (const FrameAccess&) = delete;
      inline FrameAccess(FrameAccess&& other);
      inline ~FrameAccess() noexcept;

      //  Полученный Image может быть использован только в основной очереди
      //    команд (Device::primaryQueue) и в очереди презентации.
      //  Автоматическое управление лэйаутами отключено
      //  Всегда следует полагать, что лэйаут полученного изображения
      //    VK_IMAGE_LAYOUT_UNDEFINED
      inline Image* image() const noexcept;

      //  Индекс кадра в свапчейне
      inline uint32_t frameIndex() const noexcept;

      //  Вернуть захваченный кадр обратно в свапчейн и отправить его на
      //  презентацию. Image к этому времени должен быть переведен в
      //  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      inline void present();

    private:
      SwapChain& _swapChain;
      uint32_t _frameIndex;
      Image* _image;
    };

  public:
    //  presentationMode и format можно получить из
    //    PhysicalDevice::surfaceСompatibility. Если передать nullptr, то
    //    параметры будут подобраны автоматически
    SwapChain(Device& device,
              WindowSurface& surface,
              std::optional<VkPresentModeKHR> presentationMode,
              std::optional<VkSurfaceFormatKHR> format);

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator = (const SwapChain&) = delete;
  protected:
    virtual ~SwapChain() noexcept;

  public:
    inline VkSwapchainKHR handle() const noexcept;
  
    inline VkSurfaceFormatKHR imageFormat() const noexcept;
    inline const glm::uvec2& extent() const noexcept;

    inline VkPresentModeKHR presentationMode() const noexcept;

    inline uint32_t framesCount() const noexcept;
    inline Image& frame(uint32_t frameIndex) const noexcept;

  private:
    // Захватить один кадр из свапчейна для отрисовки в нем.
    // Автоматически кидает семафор в основную очередь устройства, чтобы
    // предотвратить выполнение команд на отрисовку до готовности Image
    uint32_t lockFrame();
    // Вернуть захваченный кадр в свапчейн. Штатный вариант работы.
    // Синхронизируется с основной очередью девайса.
    // Если для image-а ещё не закончены операции с предыдущих кадров, синхронно
    // ждет их окончания
    // Ожидает, что Image кадра быдет в VK_IMAGE_LAYOUT_PRESENT_SRC_KHR лэйауте
    void presentFrame();
    // Аварийное возвращение захваченного фрэйма без синхронизацтт
    void unlockFrame() noexcept;

  private:
    void _createHandle( WindowSurface& surface,
                        std::optional<VkPresentModeKHR> presentationMode,
                        std::optional<VkSurfaceFormatKHR> format);
    void _cleanup() noexcept;

  private:
    Device& _device;
    CommandQueue* _presentQueue;
    CommandQueue* _primaryQueue;

    VkSwapchainKHR _handle;
    VkSurfaceFormatKHR _imageFormat;
    glm::uvec2 _extent;
    VkPresentModeKHR _presentationMode;

    Ref<Fence> _imageIsReadyFence;

    // Набор семафоров для синхронизации основной очереди и очереди презентации
    struct SemaphoresPair
    {
      // На этом семафоре основная очередь ждет, когда будет готов Image из
      // свапчейна
      Ref<Semaphore> startDrawing;
      // На этом семафоре очередь презентации ждет, когда картинка будет
      // нарисована, прежде чем отправить её на экран
      Ref<Semaphore> endDrawing;
    };
    std::vector<SemaphoresPair> _semaphoresPool;

    struct FrameRecord
    {
      Ref<Image> image;
      SemaphoresPair semaphores;
    };
    std::vector<FrameRecord> _frames;
    std::optional<uint32_t> _lockedFrameIndex;
  };

  inline SwapChain::FrameAccess::FrameAccess(SwapChain& swapChain):
    _swapChain(swapChain),
    _frameIndex(_swapChain.lockFrame()),
    _image(&_swapChain.frame(_frameIndex))
  {
  }

  inline SwapChain::FrameAccess::FrameAccess(FrameAccess&& other) :
    _swapChain(other._swapChain),
    _image(other._image)
  {
    other._image = nullptr;
  }

  inline void SwapChain::FrameAccess::present()
  {
    if (_image != nullptr) _swapChain.presentFrame();
    _image = nullptr;
  }

  inline SwapChain::FrameAccess::~FrameAccess()
  {
    if(_image != nullptr)
    {
      _swapChain.unlockFrame();
    }
  }

  inline Image* SwapChain::FrameAccess::image() const noexcept
  {
    return _image;
  }

  inline uint32_t SwapChain::FrameAccess::frameIndex() const noexcept
  {
    return _frameIndex;
  }

  inline VkSwapchainKHR SwapChain::handle() const noexcept
  {
    return _handle;
  }

  inline VkSurfaceFormatKHR SwapChain::imageFormat() const noexcept
  {
    return _imageFormat;
  }

  inline const glm::uvec2& SwapChain::extent() const noexcept
  {
    return _extent;
  }

  inline VkPresentModeKHR SwapChain::presentationMode() const noexcept
  {
    return _presentationMode;
  }

  inline uint32_t SwapChain::framesCount() const noexcept
  {
    return uint32_t(_frames.size());
  }

  inline Image& SwapChain::frame(uint32_t frameIndex) const noexcept
  {
    return *_frames[frameIndex].image;
  }
}