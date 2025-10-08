#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace mt
{
  struct MemoryTypeInfo;

  // Иформация об одном пуле памяти на PhysicalDevice
  struct MemoryHeapInfo
  {
    uint32_t index = 0;                   // Индекс среди всех куч устройства
    VkMemoryHeapFlags flags = 0;
    VkDeviceSize size = 0;                // Разбер в байтах

    // Указатели не могут быть nullptr
    std::vector<const MemoryTypeInfo*> types;

    inline bool isDeviceLocal() const;
  };

  // Информация об одном типе памяти, доступном на PhysicalDevice
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

  // Информация обо всех пулах и типах памяти, доступных на PhysicalDevice
  struct MemoryInfo
  {
    std::vector<MemoryHeapInfo> heaps;
    std::vector<MemoryTypeInfo> types;
  };

  inline bool MemoryHeapInfo::isDeviceLocal() const
  {
    return flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
  }

  inline bool MemoryTypeInfo::isDeviceLocal() const
  {
    return flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  }

  inline bool MemoryTypeInfo::isHostVisible() const
  {
    return flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  }

  inline bool MemoryTypeInfo::isHostCoherent() const
  {
    return flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  }

  inline bool MemoryTypeInfo::isDeviceOnly() const
  {
    return isDeviceLocal() && !isHostVisible() && !isHostCoherent();
  }
}