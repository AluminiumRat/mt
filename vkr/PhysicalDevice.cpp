#include <stdexcept>

#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>
#include <vkr/WindowSurface.h>

using namespace mt;

PhysicalDevice::PhysicalDevice(VkPhysicalDevice deviceHandle) :
  _handle(deviceHandle),
  _timelineSemaphoreSupport(false),
  _synchronization2Support(false)
{
  vkGetPhysicalDeviceProperties(_handle, &_properties);

  _getFeatures();

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

void PhysicalDevice::_getFeatures()
{
  // Сразу с фичами проверяем поддержку таймлайн семафоров
  VkPhysicalDeviceTimelineSemaphoreFeatures semaphoreFeature{};
  semaphoreFeature.sType =
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;

  // И поддержку synchronization2
  VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
  synchronization2Feature.sType =
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  synchronization2Feature.pNext = &semaphoreFeature;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &synchronization2Feature;
  vkGetPhysicalDeviceFeatures2(_handle, &features);

  _features = features.features;
  _timelineSemaphoreSupport = semaphoreFeature.timelineSemaphore;
  _synchronization2Support = synchronization2Feature.synchronization2;
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

bool PhysicalDevice::areFeaturesSupported(
                        const VkPhysicalDeviceFeatures& features) const noexcept
{
  bool featuresSupported = true;

#define VKR_CHECK_FEATURE(featureName) \
  if(features.##featureName) featuresSupported &= bool(_features.##featureName);

  VKR_CHECK_FEATURE(robustBufferAccess)
  VKR_CHECK_FEATURE(fullDrawIndexUint32)
  VKR_CHECK_FEATURE(imageCubeArray)
  VKR_CHECK_FEATURE(independentBlend)
  VKR_CHECK_FEATURE(geometryShader)
  VKR_CHECK_FEATURE(tessellationShader)
  VKR_CHECK_FEATURE(sampleRateShading)
  VKR_CHECK_FEATURE(dualSrcBlend)
  VKR_CHECK_FEATURE(logicOp)
  VKR_CHECK_FEATURE(multiDrawIndirect)
  VKR_CHECK_FEATURE(drawIndirectFirstInstance)
  VKR_CHECK_FEATURE(depthClamp)
  VKR_CHECK_FEATURE(depthBiasClamp)
  VKR_CHECK_FEATURE(fillModeNonSolid)
  VKR_CHECK_FEATURE(depthBounds)
  VKR_CHECK_FEATURE(wideLines)
  VKR_CHECK_FEATURE(largePoints)
  VKR_CHECK_FEATURE(alphaToOne)
  VKR_CHECK_FEATURE(multiViewport)
  VKR_CHECK_FEATURE(samplerAnisotropy)
  VKR_CHECK_FEATURE(textureCompressionETC2)
  VKR_CHECK_FEATURE(textureCompressionASTC_LDR)
  VKR_CHECK_FEATURE(textureCompressionBC)
  VKR_CHECK_FEATURE(occlusionQueryPrecise)
  VKR_CHECK_FEATURE(pipelineStatisticsQuery)
  VKR_CHECK_FEATURE(vertexPipelineStoresAndAtomics)
  VKR_CHECK_FEATURE(fragmentStoresAndAtomics)
  VKR_CHECK_FEATURE(shaderTessellationAndGeometryPointSize)
  VKR_CHECK_FEATURE(shaderImageGatherExtended)
  VKR_CHECK_FEATURE(shaderStorageImageExtendedFormats)
  VKR_CHECK_FEATURE(shaderStorageImageMultisample)
  VKR_CHECK_FEATURE(shaderStorageImageReadWithoutFormat)
  VKR_CHECK_FEATURE(shaderStorageImageWriteWithoutFormat)
  VKR_CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing)
  VKR_CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing)
  VKR_CHECK_FEATURE(shaderClipDistance)
  VKR_CHECK_FEATURE(shaderCullDistance)
  VKR_CHECK_FEATURE(shaderFloat64)
  VKR_CHECK_FEATURE(shaderInt64)
  VKR_CHECK_FEATURE(shaderInt16)
  VKR_CHECK_FEATURE(shaderResourceResidency)
  VKR_CHECK_FEATURE(shaderResourceMinLod)
  VKR_CHECK_FEATURE(sparseBinding)
  VKR_CHECK_FEATURE(sparseResidencyBuffer)
  VKR_CHECK_FEATURE(sparseResidencyImage2D)
  VKR_CHECK_FEATURE(sparseResidencyImage3D)
  VKR_CHECK_FEATURE(sparseResidency2Samples)
  VKR_CHECK_FEATURE(sparseResidency4Samples)
  VKR_CHECK_FEATURE(sparseResidency8Samples)
  VKR_CHECK_FEATURE(sparseResidency16Samples)
  VKR_CHECK_FEATURE(sparseResidencyAliased)
  VKR_CHECK_FEATURE(variableMultisampleRate)
  VKR_CHECK_FEATURE(inheritedQueries)

#undef VKR_CHECK_FEATURE

  return featuresSupported;
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
