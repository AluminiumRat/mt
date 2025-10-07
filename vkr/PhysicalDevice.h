#pragma once

#include <cstdlib>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/queue/QueueFamily.h>

namespace mt
{
  class WindowSurface;

  class PhysicalDevice
  {
  public:
    struct MemoryTypeInfo;

    struct MemoryHeapInfo
    {
      uint32_t index = 0;                   // Индекс среди всех куч устройства
      VkMemoryHeapFlags flags = 0;
      VkDeviceSize size = 0;                // Разбер в байтах

      // Указатели не могут быть nullptr
      std::vector<const MemoryTypeInfo*> types;

      inline bool isDeviceLocal() const;
    };

    struct MemoryTypeInfo
    {
      uint32_t index = 0;                   // Индекс среди всех типов девайса
      VkMemoryPropertyFlags flags = 0;
      const MemoryHeapInfo* heap;           // heap не может быть nullptr

      inline bool isDeviceLocal() const;
      inline bool isHostVisible() const;
      inline bool isHostCoherent() const;
      // Память находится на устройстве и напрямую не доступна с CPU
      inline bool isDeviceOnly() const;
    };

    struct MemoryInfo
    {
      std::vector<MemoryHeapInfo> heaps;
      std::vector<MemoryTypeInfo> types;
    };

    struct SwapChainSupport
    {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    };

    using QueuesInfo = std::vector<QueueFamily>;

  public:
    PhysicalDevice(VkPhysicalDevice deviceHandle);
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator = (const PhysicalDevice&) = delete;
    virtual ~PhysicalDevice() noexcept = default;

    inline VkPhysicalDevice handle() const noexcept;

    inline const VkPhysicalDeviceProperties& properties() const noexcept;
    inline const VkPhysicalDeviceFeatures& features() const noexcept;
    inline const MemoryInfo& memoryInfo() const noexcept;

    std::vector<VkExtensionProperties> availableExtensions() const;
    inline bool isExtensionSupported(const char* extensionName) const;

    inline QueuesInfo queuesInfo() const noexcept;

    SwapChainSupport surfaceСompatibility(const WindowSurface& surface) const;
    inline bool isSurfaceSuitable(const WindowSurface& surface) const;

  private:
    void _fillMemoryInfo();

  private:
    VkPhysicalDevice _handle;

    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceFeatures _features;
    MemoryInfo _memoryInfo;
    QueuesInfo _queuesInfo;
  };

  inline bool PhysicalDevice::MemoryHeapInfo::isDeviceLocal() const
  {
    return flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
  }

  inline bool PhysicalDevice::MemoryTypeInfo::isDeviceLocal() const
  {
    return flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  }

  inline bool PhysicalDevice::MemoryTypeInfo::isHostVisible() const
  {
    return flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  }

  inline bool PhysicalDevice::MemoryTypeInfo::isHostCoherent() const
  {
    return flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  }

  inline bool PhysicalDevice::MemoryTypeInfo::isDeviceOnly() const
  {
    return isDeviceLocal() && !isHostVisible() && !isHostCoherent();
  }

  inline VkPhysicalDevice PhysicalDevice::handle() const noexcept
  {
    return _handle;
  }

  inline const VkPhysicalDeviceProperties&
                                    PhysicalDevice::properties() const noexcept
  {
    return _properties;
  }

  inline const VkPhysicalDeviceFeatures&
                                      PhysicalDevice::features() const noexcept
  {
    return _features;
  }

  inline const PhysicalDevice::MemoryInfo&
                                    PhysicalDevice::memoryInfo() const noexcept
  {
    return _memoryInfo;
  }

  inline bool PhysicalDevice::isExtensionSupported(
                                                const char* extensionName) const
  {
    for (const VkExtensionProperties& extension : availableExtensions())
    {
      if (strcmp(extensionName, extension.extensionName) == 0) return true;
    }
    return false;
  }

  inline PhysicalDevice::QueuesInfo PhysicalDevice::queuesInfo() const noexcept
  {
    return _queuesInfo;
  }

  inline bool PhysicalDevice::isSurfaceSuitable(
                                            const WindowSurface& surface) const
  {
    SwapChainSupport support = surfaceСompatibility(surface);
    return support.formats.size() != 0 && support.presentModes.size() != 0;
  }
}