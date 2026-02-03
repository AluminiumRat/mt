#include <stdexcept>

#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>
#include <vkr/WindowSurface.h>

using namespace mt;

PhysicalDevice::PhysicalDevice(VkPhysicalDevice deviceHandle) :
  _handle(deviceHandle)
{
  _getProperties();
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

void PhysicalDevice::_getProperties()
{
  _properties._properties14 = VkPhysicalDeviceVulkan14Properties{};
  _properties._properties14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;

  _properties._properties13 = VkPhysicalDeviceVulkan13Properties{};
  _properties._properties13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
  _properties._properties13.pNext = &_properties._properties14;

  _properties._properties12 = VkPhysicalDeviceVulkan12Properties{};
  _properties._properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
  _properties._properties12.pNext = &_properties._properties13;

  _properties._properties11 = VkPhysicalDeviceVulkan11Properties{};
  _properties._properties11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
  _properties._properties11.pNext = &_properties._properties12;

  VkPhysicalDeviceProperties2 properties2{};
  properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties2.pNext = &_properties._properties11;

  vkGetPhysicalDeviceProperties2(_handle, &properties2);

  _properties._properties10 = properties2.properties;
}

void PhysicalDevice::_getFeatures()
{
  _features.features14 = VkPhysicalDeviceVulkan14Features{};
  _features.features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

  _features.features13 = VkPhysicalDeviceVulkan13Features{};
  _features.features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  _features.features13.pNext = &_features.features14;

  _features.features12 = VkPhysicalDeviceVulkan12Features{};
  _features.features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  _features.features12.pNext = &_features.features13;

  _features.features11 = VkPhysicalDeviceVulkan11Features{};
  _features.features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  _features.features11.pNext = &_features.features12;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &_features.features11;

  vkGetPhysicalDeviceFeatures2(_handle, &features);

  _features.features10 = features.features;
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
                                      const Features& features) const noexcept
{
  bool featuresSupported = true;

  #define VKR_CHECK_FEATURE_10(featureName) \
    if(features.features10.##featureName) featuresSupported &= bool(_features.features10.##featureName);

  VKR_CHECK_FEATURE_10(robustBufferAccess)
  VKR_CHECK_FEATURE_10(fullDrawIndexUint32)
  VKR_CHECK_FEATURE_10(imageCubeArray)
  VKR_CHECK_FEATURE_10(independentBlend)
  VKR_CHECK_FEATURE_10(geometryShader)
  VKR_CHECK_FEATURE_10(tessellationShader)
  VKR_CHECK_FEATURE_10(sampleRateShading)
  VKR_CHECK_FEATURE_10(dualSrcBlend)
  VKR_CHECK_FEATURE_10(logicOp)
  VKR_CHECK_FEATURE_10(multiDrawIndirect)
  VKR_CHECK_FEATURE_10(drawIndirectFirstInstance)
  VKR_CHECK_FEATURE_10(depthClamp)
  VKR_CHECK_FEATURE_10(depthBiasClamp)
  VKR_CHECK_FEATURE_10(fillModeNonSolid)
  VKR_CHECK_FEATURE_10(depthBounds)
  VKR_CHECK_FEATURE_10(wideLines)
  VKR_CHECK_FEATURE_10(largePoints)
  VKR_CHECK_FEATURE_10(alphaToOne)
  VKR_CHECK_FEATURE_10(multiViewport)
  VKR_CHECK_FEATURE_10(samplerAnisotropy)
  VKR_CHECK_FEATURE_10(textureCompressionETC2)
  VKR_CHECK_FEATURE_10(textureCompressionASTC_LDR)
  VKR_CHECK_FEATURE_10(textureCompressionBC)
  VKR_CHECK_FEATURE_10(occlusionQueryPrecise)
  VKR_CHECK_FEATURE_10(pipelineStatisticsQuery)
  VKR_CHECK_FEATURE_10(vertexPipelineStoresAndAtomics)
  VKR_CHECK_FEATURE_10(fragmentStoresAndAtomics)
  VKR_CHECK_FEATURE_10(shaderTessellationAndGeometryPointSize)
  VKR_CHECK_FEATURE_10(shaderImageGatherExtended)
  VKR_CHECK_FEATURE_10(shaderStorageImageExtendedFormats)
  VKR_CHECK_FEATURE_10(shaderStorageImageMultisample)
  VKR_CHECK_FEATURE_10(shaderStorageImageReadWithoutFormat)
  VKR_CHECK_FEATURE_10(shaderStorageImageWriteWithoutFormat)
  VKR_CHECK_FEATURE_10(shaderUniformBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE_10(shaderSampledImageArrayDynamicIndexing)
  VKR_CHECK_FEATURE_10(shaderStorageBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE_10(shaderStorageImageArrayDynamicIndexing)
  VKR_CHECK_FEATURE_10(shaderClipDistance)
  VKR_CHECK_FEATURE_10(shaderCullDistance)
  VKR_CHECK_FEATURE_10(shaderFloat64)
  VKR_CHECK_FEATURE_10(shaderInt64)
  VKR_CHECK_FEATURE_10(shaderInt16)
  VKR_CHECK_FEATURE_10(shaderResourceResidency)
  VKR_CHECK_FEATURE_10(shaderResourceMinLod)
  VKR_CHECK_FEATURE_10(sparseBinding)
  VKR_CHECK_FEATURE_10(sparseResidencyBuffer)
  VKR_CHECK_FEATURE_10(sparseResidencyImage2D)
  VKR_CHECK_FEATURE_10(sparseResidencyImage3D)
  VKR_CHECK_FEATURE_10(sparseResidency2Samples)
  VKR_CHECK_FEATURE_10(sparseResidency4Samples)
  VKR_CHECK_FEATURE_10(sparseResidency8Samples)
  VKR_CHECK_FEATURE_10(sparseResidency16Samples)
  VKR_CHECK_FEATURE_10(sparseResidencyAliased)
  VKR_CHECK_FEATURE_10(variableMultisampleRate)
  VKR_CHECK_FEATURE_10(inheritedQueries)

  #define VKR_CHECK_FEATURE_11(featureName) \
    if(features.features11.##featureName) featuresSupported &= bool(_features.features11.##featureName);
  VKR_CHECK_FEATURE_11(storageBuffer16BitAccess)
  VKR_CHECK_FEATURE_11(uniformAndStorageBuffer16BitAccess)
  VKR_CHECK_FEATURE_11(storagePushConstant16)
  VKR_CHECK_FEATURE_11(storageInputOutput16)
  VKR_CHECK_FEATURE_11(multiview)
  VKR_CHECK_FEATURE_11(multiviewGeometryShader)
  VKR_CHECK_FEATURE_11(multiviewTessellationShader)
  VKR_CHECK_FEATURE_11(variablePointersStorageBuffer)
  VKR_CHECK_FEATURE_11(variablePointers)
  VKR_CHECK_FEATURE_11(protectedMemory)
  VKR_CHECK_FEATURE_11(samplerYcbcrConversion)
  VKR_CHECK_FEATURE_11(shaderDrawParameters)

  #define VKR_CHECK_FEATURE_12(featureName) \
    if(features.features12.##featureName) featuresSupported &= bool(_features.features12.##featureName);

  VKR_CHECK_FEATURE_12(samplerMirrorClampToEdge)
  VKR_CHECK_FEATURE_12(drawIndirectCount)
  VKR_CHECK_FEATURE_12(storageBuffer8BitAccess)
  VKR_CHECK_FEATURE_12(uniformAndStorageBuffer8BitAccess)
  VKR_CHECK_FEATURE_12(storagePushConstant8)
  VKR_CHECK_FEATURE_12(shaderBufferInt64Atomics)
  VKR_CHECK_FEATURE_12(shaderSharedInt64Atomics)
  VKR_CHECK_FEATURE_12(shaderFloat16)
  VKR_CHECK_FEATURE_12(shaderInt8)
  VKR_CHECK_FEATURE_12(descriptorIndexing)
  VKR_CHECK_FEATURE_12(shaderInputAttachmentArrayDynamicIndexing)
  VKR_CHECK_FEATURE_12(shaderUniformTexelBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE_12(shaderStorageTexelBufferArrayDynamicIndexing)
  VKR_CHECK_FEATURE_12(shaderUniformBufferArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderSampledImageArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderStorageBufferArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderStorageImageArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderInputAttachmentArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderUniformTexelBufferArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(shaderStorageTexelBufferArrayNonUniformIndexing)
  VKR_CHECK_FEATURE_12(descriptorBindingUniformBufferUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingSampledImageUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingStorageImageUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingStorageBufferUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingUniformTexelBufferUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingStorageTexelBufferUpdateAfterBind)
  VKR_CHECK_FEATURE_12(descriptorBindingUpdateUnusedWhilePending)
  VKR_CHECK_FEATURE_12(descriptorBindingPartiallyBound)
  VKR_CHECK_FEATURE_12(descriptorBindingVariableDescriptorCount)
  VKR_CHECK_FEATURE_12(runtimeDescriptorArray)
  VKR_CHECK_FEATURE_12(samplerFilterMinmax)
  VKR_CHECK_FEATURE_12(scalarBlockLayout)
  VKR_CHECK_FEATURE_12(imagelessFramebuffer)
  VKR_CHECK_FEATURE_12(uniformBufferStandardLayout)
  VKR_CHECK_FEATURE_12(shaderSubgroupExtendedTypes)
  VKR_CHECK_FEATURE_12(separateDepthStencilLayouts)
  VKR_CHECK_FEATURE_12(hostQueryReset)
  VKR_CHECK_FEATURE_12(timelineSemaphore)
  VKR_CHECK_FEATURE_12(bufferDeviceAddress)
  VKR_CHECK_FEATURE_12(bufferDeviceAddressCaptureReplay)
  VKR_CHECK_FEATURE_12(bufferDeviceAddressMultiDevice)
  VKR_CHECK_FEATURE_12(vulkanMemoryModel)
  VKR_CHECK_FEATURE_12(vulkanMemoryModelDeviceScope)
  VKR_CHECK_FEATURE_12(vulkanMemoryModelAvailabilityVisibilityChains)
  VKR_CHECK_FEATURE_12(shaderOutputViewportIndex)
  VKR_CHECK_FEATURE_12(shaderOutputLayer)
  VKR_CHECK_FEATURE_12(subgroupBroadcastDynamicId)

  #define VKR_CHECK_FEATURE_13(featureName) \
    if(features.features13.##featureName) featuresSupported &= bool(_features.features13.##featureName);
  VKR_CHECK_FEATURE_13(robustImageAccess)
  VKR_CHECK_FEATURE_13(inlineUniformBlock)
  VKR_CHECK_FEATURE_13(descriptorBindingInlineUniformBlockUpdateAfterBind)
  VKR_CHECK_FEATURE_13(pipelineCreationCacheControl)
  VKR_CHECK_FEATURE_13(privateData)
  VKR_CHECK_FEATURE_13(shaderDemoteToHelperInvocation)
  VKR_CHECK_FEATURE_13(shaderTerminateInvocation)
  VKR_CHECK_FEATURE_13(subgroupSizeControl)
  VKR_CHECK_FEATURE_13(computeFullSubgroups)
  VKR_CHECK_FEATURE_13(synchronization2)
  VKR_CHECK_FEATURE_13(textureCompressionASTC_HDR)
  VKR_CHECK_FEATURE_13(shaderZeroInitializeWorkgroupMemory)
  VKR_CHECK_FEATURE_13(dynamicRendering)
  VKR_CHECK_FEATURE_13(shaderIntegerDotProduct)
  VKR_CHECK_FEATURE_13(maintenance4)

  #define VKR_CHECK_FEATURE_14(featureName) \
    if(features.features14.##featureName) featuresSupported &= bool(_features.features14.##featureName);
  VKR_CHECK_FEATURE_14(globalPriorityQuery)
  VKR_CHECK_FEATURE_14(shaderSubgroupRotate)
  VKR_CHECK_FEATURE_14(shaderSubgroupRotateClustered)
  VKR_CHECK_FEATURE_14(shaderFloatControls2)
  VKR_CHECK_FEATURE_14(shaderExpectAssume)
  VKR_CHECK_FEATURE_14(rectangularLines)
  VKR_CHECK_FEATURE_14(bresenhamLines)
  VKR_CHECK_FEATURE_14(smoothLines)
  VKR_CHECK_FEATURE_14(stippledRectangularLines)
  VKR_CHECK_FEATURE_14(stippledBresenhamLines)
  VKR_CHECK_FEATURE_14(stippledSmoothLines)
  VKR_CHECK_FEATURE_14(vertexAttributeInstanceRateDivisor)
  VKR_CHECK_FEATURE_14(vertexAttributeInstanceRateZeroDivisor)
  VKR_CHECK_FEATURE_14(indexTypeUint8)
  VKR_CHECK_FEATURE_14(dynamicRenderingLocalRead)
  VKR_CHECK_FEATURE_14(maintenance5)
  VKR_CHECK_FEATURE_14(maintenance6)
  VKR_CHECK_FEATURE_14(pipelineProtectedAccess)
  VKR_CHECK_FEATURE_14(pipelineRobustness)
  VKR_CHECK_FEATURE_14(hostImageCopy)
  VKR_CHECK_FEATURE_14(pushDescriptor)

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
