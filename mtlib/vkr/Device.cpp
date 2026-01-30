#include <array>
#include <stdexcept>

#include <util/Abort.h>
#include <util/Assert.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>

using namespace mt;

Device::Device( PhysicalDevice& physicalDevice,
                VkPhysicalDeviceFeatures requiredFeatures,
                VkPhysicalDeviceVulkan12Features requiredFeaturesVulkan12,
                const std::vector<std::string>& requiredExtensions,
                const std::vector<std::string>& enabledLayers,
                const QueueSources& queueSources) :
  _physicalDevice(physicalDevice),
  _handle(VK_NULL_HANDLE),
  _allocator(VK_NULL_HANDLE),
  _features(requiredFeatures),
  _featuresVulkan12(requiredFeaturesVulkan12),
  _queuesByTypes{}
{
  try
  {
    _createHandle(requiredExtensions, enabledLayers, queueSources);
    _initVMA();
    _buildQueues(queueSources);
    _extFunctions.emplace(_handle);
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

void Device::_createHandle( const std::vector<std::string>& requiredExtensions,
                            const std::vector<std::string>& enabledLayers,
                            const QueueSources& queueSources)
{
  // Определяем, для каких семейств сколько очередей надо создать
  std::vector<uint32_t> queueCounters(_physicalDevice.queuesInfo().size(), 0);
  for(const QueueSource& source : queueSources)
  {
    // Проверяем, что в источнике очереди находится указатель на семейство
    // очередей. Это значит, что нужно создать уникальную очередь
    if(const QueueFamily* const * family =
                                      std::get_if<const QueueFamily*>(&source))
    {
      queueCounters[(*family)->index()]++;
    }
  }

  // Заполняем инфу об очередях для вулкана
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::array<float, 4> priorities = { 1.f, 1.f, 1.f, 1.f };
  for(uint32_t familyIndex = 0;
      familyIndex < queueCounters.size();
      familyIndex++)
  {
    if(queueCounters[familyIndex] != 0)
    {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = familyIndex;
      queueCreateInfo.queueCount = queueCounters[familyIndex];
      queueCreateInfo.pQueuePriorities = priorities.data();
      queueCreateInfos.push_back(queueCreateInfo);
    }
  }

  // Обязательно включить synchronization2
  VkPhysicalDeviceSynchronization2Features synchronization2Feature{};
  synchronization2Feature.sType =
                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  synchronization2Feature.synchronization2 = VK_TRUE;

  // DynamicRendering нужен для графической конфигурации, сильно упрощает
  // работу с пайплайном
  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature{};
  dynamicRenderingFeature.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
  dynamicRenderingFeature.dynamicRendering = VK_TRUE;
  dynamicRenderingFeature.pNext = &synchronization2Feature;

  _featuresVulkan12.sType =
                          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  _featuresVulkan12.pNext = &dynamicRenderingFeature;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.features = _features;
  features.pNext = &_featuresVulkan12;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
  createInfo.pNext = &features;

  // Заполняем инфу об расширениях
  std::vector<const char*> extensions;
  for(const std::string& extension : requiredExtensions)
  {
    extensions.push_back(extension.c_str());
  }
  if(!extensions.empty())
  {
    createInfo.enabledExtensionCount = uint32_t(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
  }

  // Заполняем инфу об вулкан лэйерах
  std::vector<const char*> layers;
  for (const std::string& layer : enabledLayers)
  {
    layers.push_back(layer.c_str());
  }
  if (!layers.empty())
  {
    createInfo.enabledLayerCount = uint32_t(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();
  }

  // Создаем сам девайс
  if (vkCreateDevice( _physicalDevice.handle(),
                      &createInfo,
                      nullptr,
                      &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error("Device: Failed to create logical device.");
  }
}

void Device::_initVMA()
{
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion =
                          VKRLib::instance().vulkanApiVersion();
  allocatorInfo.physicalDevice = _physicalDevice.handle();
  allocatorInfo.device = _handle;
  allocatorInfo.instance = VKRLib::instance().handle();
  allocatorInfo.vulkanApiVersion = VKRLib::instance().vulkanApiVersion();
  if(vmaCreateAllocator(&allocatorInfo, &_allocator) != VK_SUCCESS)
  {
    throw std::runtime_error("Device: Failed to create memory allocator.");
  }
}

void Device::_buildQueues(const QueueSources& queueSources)
{
  // Счетчик, сколько для какого семейства уже создано очередей
  std::vector<uint32_t> queueCounters(_physicalDevice.queuesInfo().size(), 0);

  // Для начала обходим все типы очередей и создаем уникальные очереди
  for(int queueTypeIndex = 0; queueTypeIndex < QueueTypeCount; queueTypeIndex++)
  {
    // Проверяем, нужно ли создавать отдельный объект CommandQueue для этого
    // типа очереди.
    const QueueSource& source = queueSources[queueTypeIndex];
    if (const QueueFamily* const* family =
                                      std::get_if<const QueueFamily*>(&source))
    {
      uint32_t familyIndex = (*family)->index();
      std::unique_ptr<CommandQueue> newQueue;
      switch(queueTypeIndex)
      {
        case GRAPHIC_QUEUE:
          newQueue.reset(new CommandQueueGraphic( *this,
                                                  familyIndex,
                                                  queueCounters[familyIndex],
                                                  **family,
                                                  _commonQueuesMutex));
          break;
        case COMPUTE_QUEUE:
          newQueue.reset(new CommandQueueCompute( *this,
                                                  familyIndex,
                                                  queueCounters[familyIndex],
                                                  **family,
                                                  _commonQueuesMutex));
          break;
        case PRESENTATION_QUEUE:
          newQueue.reset(new CommandQueue(*this,
                                          familyIndex,
                                          queueCounters[familyIndex],
                                          **family,
                                          _commonQueuesMutex));
          break;
        case TRANSFER_QUEUE:
          newQueue.reset(new CommandQueueTransfer(*this,
                                                  familyIndex,
                                                  queueCounters[familyIndex],
                                                  **family,
                                                  _commonQueuesMutex));
          break;
      }
      MT_ASSERT(newQueue != nullptr);
      queueCounters[familyIndex]++;
      _queuesByTypes[queueTypeIndex] = newQueue.get();
      _queues.push_back(std::move(newQueue));
    }
  }

  // Теперь, когда все уникальные очереди уже созданы, мы можем ещё раз пройтись
  // по всем типам очередей и раскидать указатели в тех случаях, когда вместо
  // выделенной очереди используется какая-то ещё
  for (int queueTypeIndex = 0; queueTypeIndex < QueueTypeCount; queueTypeIndex++)
  {
    // Если в сорсе указан другой тип очереди, значит та очередь должна
    // использоваться по нескольким назначениям. Например, графическая очередь
    // будет использоваться ещё и как трансфер.
    const QueueSource& source = queueSources[queueTypeIndex];
    if (const QueueType* alternateType = std::get_if<QueueType>(&source))
    {
      CommandQueue* alternate = _queuesByTypes[*alternateType];
      MT_ASSERT((alternate != nullptr) && "Device::_buildQueues: Alternate queue is not created");
      _queuesByTypes[queueTypeIndex] = alternate;
    }
  }
}

Device::~Device() noexcept
{
  _cleanup();
}

void Device::_cleanup() noexcept
{
  // Перед удалением остановим все очереди
  for(std::unique_ptr<CommandQueue>& queue : _queues)
  {
    try
    {
      queue->waitIdle();
    }
    catch (std::exception& error)
    {
      Log::error() << error.what();
    }
  }

  _queuesByTypes = {};
  _queues.clear();

  if (_allocator != VK_NULL_HANDLE)
  {
    vmaDestroyAllocator(_allocator);
    _allocator = VK_NULL_HANDLE;
  }

  if (_handle != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(_handle);
    vkDestroyDevice(_handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}

bool Device::isSurfaceSuitable(const WindowSurface& surface) const
{
  if(!_physicalDevice.isSurfaceSuitable(surface)) return false;

  if(_queuesByTypes[PRESENTATION_QUEUE] == nullptr) return false;

  return
      _queuesByTypes[PRESENTATION_QUEUE]->family().isPresentSupported(surface);
}

std::unique_lock<std::recursive_mutex> Device::lockQueues() noexcept
{
  std::unique_lock<std::recursive_mutex> locker(_commonQueuesMutex);
  if(vkDeviceWaitIdle(_handle) != VK_SUCCESS)
  {
    Abort("Device::lockQueues: unable to wait for device");
  }
  return locker;
}
