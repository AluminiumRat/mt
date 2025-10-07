#include <stdexcept>

#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>
#include <vkr/WindowSurface.h>

using namespace mt;

PhysicalDevice::PhysicalDevice(VkPhysicalDevice deviceHandle) :
  _handle(deviceHandle)
{
  vkGetPhysicalDeviceProperties(_handle, &_properties);
  vkGetPhysicalDeviceFeatures(_handle, &_features);

  _fillMemoryInfo();

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(_handle, &queueFamilyCount, nullptr);
  _queuesInfo.reserve(queueFamilyCount);
  for(uint32_t queueFamilyIndex = 0;
      queueFamilyIndex < queueFamilyCount;
      queueFamilyIndex++)
  {
    _queuesInfo.emplace_back(*this, queueFamilyIndex);
  }
}

void PhysicalDevice::_fillMemoryInfo()
{
  // Получаем информацию о памяти, доступной на устройстве
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(_handle, &memoryProperties);

  if( memoryProperties.memoryTypeCount == 0 ||
      memoryProperties.memoryHeapCount == 0)
  {
    throw std::runtime_error("PhysicalDevice: Unable to get devices memory properties.");
  }

  // Получаем информацию по кучам
  _memoryInfo.heaps.reserve(memoryProperties.memoryHeapCount);
  for(uint32_t heapIndex = 0;
      heapIndex < memoryProperties.memoryHeapCount;
      heapIndex++)
  {
    MemoryHeapInfo info;
    info.index = heapIndex;
    info.flags = memoryProperties.memoryHeaps[heapIndex].flags;
    info.size = memoryProperties.memoryHeaps[heapIndex].size;
    _memoryInfo.heaps.push_back(info);
  }

  // Получаем информацию по тиам памяти
  _memoryInfo.types.reserve(memoryProperties.memoryTypeCount);
  for(uint32_t typeIndex = 0;
      typeIndex < memoryProperties.memoryTypeCount;
      typeIndex++)
  {
    MemoryTypeInfo info;
    info.index = typeIndex;
    info.flags = memoryProperties.memoryTypes[typeIndex].propertyFlags;
    uint32_t heapIndex = memoryProperties.memoryTypes[typeIndex].heapIndex;
    info.heap = &_memoryInfo.heaps[heapIndex];
    _memoryInfo.types.push_back(info);
  }

  // Раскидываем ссылки на типы по кучам
  for(MemoryTypeInfo& type : _memoryInfo.types)
  {
    _memoryInfo.heaps[type.heap->index].types.push_back(&type);
  }
}

PhysicalDevice::SwapChainSupport PhysicalDevice::surfaceСompatibility(
                                            const WindowSurface& surface) const
{
  SwapChainSupport support{};

  // Проверяем, что хотя бы одно семейство очередей поддерживало пресент в
  // surface
  bool presentIsSupported = false;
  for(const QueueFamily& queuesFamily : _queuesInfo)
  {
    presentIsSupported |= queuesFamily.isPresentSupported(surface);
  }
  if(!presentIsSupported) return support;

  // Получаем ограничения и возможности свапчейна
  if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                                        _handle,
                                        surface.handle(),
                                        &support.capabilities) != VK_SUCCESS)
  {
    throw std::runtime_error("PhysicalDevice: Unable to get surface capabilities.");
  }

  // Получаем поддерживаемые форматы
  uint32_t formatCount;
  if(vkGetPhysicalDeviceSurfaceFormatsKHR( _handle,
                                          surface.handle(),
                                          &formatCount,
                                          nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("PhysicalDevice: Unable to get surface format count.");
  }
  if (formatCount != 0)
  {
    support.formats.resize(formatCount);
    if(vkGetPhysicalDeviceSurfaceFormatsKHR(
                                        _handle,
                                        surface.handle(),
                                        &formatCount,
                                        support.formats.data()) != VK_SUCCESS)
    {
      throw std::runtime_error("PhysicalDevice: Unable to get surface formats.");
    }
  }

  // Получаем поддерживаемые режимы
  uint32_t presentModeCount;
  if(vkGetPhysicalDeviceSurfacePresentModesKHR( _handle,
                                                surface.handle(),
                                                &presentModeCount,
                                                nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("PhysicalDevice: Unable to get surface present mode count.");
  }
  if (presentModeCount != 0)
  {
    support.presentModes.resize(presentModeCount);
    if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                                    _handle,
                                    surface.handle(),
                                    &presentModeCount,
                                    support.presentModes.data()) != VK_SUCCESS)
    {
      throw std::runtime_error("PhysicalDevice: Unable to get surface present modes.");
    }
  }

  return support;
}

std::vector<VkExtensionProperties> PhysicalDevice::availableExtensions() const
{
  uint32_t extensionsCount = 0;
  if (vkEnumerateDeviceExtensionProperties(
        _handle,
        nullptr,
        &extensionsCount,
        nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("PhysicalDevice: Unable to get vk extensions count.");
  }

  if (extensionsCount == 0) return {};

  std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
  if (vkEnumerateDeviceExtensionProperties(
        _handle,
        nullptr,
        &extensionsCount,
        availableExtensions.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("PhysicalDevice: Unable to enumerate vk extensions.");
  }

  return availableExtensions;
}