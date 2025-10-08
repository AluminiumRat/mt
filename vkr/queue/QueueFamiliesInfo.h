#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  class QueueFamily;
  class PhysicalDevice;
  class WindowSurface;

  // Информация обо всех доступных семействах очередей на PhisicalDevice
  using QueueFamiliesInfo = std::vector<QueueFamily>;

  // Вспомогательный класс для работы с инфорацией о семейсве очередей на
  // физическом устройстве. Просто упрощенный способ получить информацию
  class QueueFamily
  {
  public:
    QueueFamily(PhysicalDevice& device, uint32_t familyIndex);
    QueueFamily(const QueueFamily&) = default;
    QueueFamily& operator = (const QueueFamily&) = default;
    ~QueueFamily() noexcept = default;

    inline PhysicalDevice& device() const noexcept;
    inline uint32_t index() const noexcept;

    inline VkQueueFlags queueFlags() const noexcept;
    // Очередь поддерживает и графические операции, и компьюты и трансфер
    inline bool isGraphic() const noexcept;
    // Поддерживает компьюты и трансфер
    inline bool isCompute() const noexcept;
    // Очередь поддерживает компьюты и трансфер, но не поддерживает графические
    // операции.
    inline bool isSeparateCompute() const noexcept;
    // Поддерживает трансфер
    inline bool isTransfer() const noexcept;
    // Очередь поддерживает трансфер, но не поддерживает ни графические операции
    // ни компьюты
    inline bool isSeparateTransfer() const noexcept;
    inline bool sparseBindingSupported() const noexcept;

    inline uint32_t queueCount() const noexcept;
    inline uint32_t timestampValidBits() const noexcept;
    inline VkExtent3D minImageTransferGranularity() const noexcept;

    bool isPresentSupported(const WindowSurface& surface) const;

  private:
    PhysicalDevice* _device;
    uint32_t _index;

    VkQueueFamilyProperties _properties;
  };

  inline uint32_t QueueFamily::index() const noexcept
  {
    return _index;
  }

  inline PhysicalDevice& QueueFamily::device() const noexcept
  {
    return *_device;
  }

  inline VkQueueFlags QueueFamily::queueFlags() const noexcept
  {
    return _properties.queueFlags;
  }

  inline bool QueueFamily::isGraphic() const noexcept
  {
    return  (queueFlags() & VK_QUEUE_GRAPHICS_BIT) &&
            (queueFlags() & VK_QUEUE_COMPUTE_BIT) &&
            (queueFlags() & VK_QUEUE_TRANSFER_BIT);
  }

  inline bool QueueFamily::isCompute() const noexcept
  {
    return  (queueFlags() & VK_QUEUE_COMPUTE_BIT) &&
            (queueFlags() & VK_QUEUE_TRANSFER_BIT);
  }

  inline bool QueueFamily::isSeparateCompute() const noexcept
  {
    return  (queueFlags() & VK_QUEUE_COMPUTE_BIT) &&
            (queueFlags() & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFlags() & VK_QUEUE_GRAPHICS_BIT);
  }

  inline bool QueueFamily::isTransfer() const noexcept
  {
    return queueFlags() & VK_QUEUE_TRANSFER_BIT;
  }

  inline bool QueueFamily::isSeparateTransfer() const noexcept
  {
    return  (queueFlags() & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFlags() & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFlags() & VK_QUEUE_COMPUTE_BIT);
  }

  inline bool QueueFamily::sparseBindingSupported() const noexcept
  {
    return queueFlags() & VK_QUEUE_SPARSE_BINDING_BIT;
  }

  inline uint32_t QueueFamily::queueCount() const noexcept
  {
    return _properties.queueCount;
  }

  inline uint32_t QueueFamily::timestampValidBits() const noexcept
  {
    return _properties.timestampValidBits;
  }

  inline VkExtent3D QueueFamily::minImageTransferGranularity() const noexcept
  {
    return _properties.minImageTransferGranularity;
  }
}