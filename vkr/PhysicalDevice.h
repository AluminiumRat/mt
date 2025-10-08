#pragma once

#include <cstdlib>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/queue/QueueFamiliesInfo.h>
#include <vkr/MemoryInfo.h>

namespace mt
{
  class WindowSurface;

  // Класс предоставляет информацию об одном из графических устройств,
  // установленном в системе.
  // Список всех устройств можно получить через класс VKRLib
  class PhysicalDevice
  {
  public:
    struct SwapChainSupport
    {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    };

  private:
    // Только VKRLib может создавать объекты этого класса
    friend class VKRLib;
    PhysicalDevice(VkPhysicalDevice deviceHandle);

  public:
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator = (const PhysicalDevice&) = delete;
    virtual ~PhysicalDevice() noexcept = default;

    inline VkPhysicalDevice handle() const noexcept;

    inline const VkPhysicalDeviceProperties& properties() const noexcept;

    inline const VkPhysicalDeviceFeatures& features() const noexcept;
    bool areFeaturesSupported(
                      const VkPhysicalDeviceFeatures& features) const noexcept;

    inline const MemoryInfo& memoryInfo() const noexcept;

    std::vector<VkExtensionProperties> availableExtensions() const;
    inline bool isExtensionSupported(const char* extensionName) const;

    inline const QueueFamiliesInfo& queuesInfo() const noexcept;

    SwapChainSupport surfaceСompatibility(const WindowSurface& surface) const;
    inline bool isSurfaceSuitable(const WindowSurface& surface) const;

  private:
    void _fillMemoryInfo();

  private:
    VkPhysicalDevice _handle;

    VkPhysicalDeviceProperties _properties;
    VkPhysicalDeviceFeatures _features;
    MemoryInfo _memoryInfo;
    QueueFamiliesInfo _queuesInfo;
  };

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

  inline const MemoryInfo& PhysicalDevice::memoryInfo() const noexcept
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

  inline const QueueFamiliesInfo& PhysicalDevice::queuesInfo() const noexcept
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