#include <array>
#include <stdexcept>

#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>
#include <util/Assert.h>

using namespace mt;

/*const size_t DRAW_POOL_SIZE = 5;
const size_t TRANSFER_POOL_SIZE = 2;*/

Device::Device( PhysicalDevice& physicalDevice,
                VkPhysicalDeviceFeatures requiredFeatures,
                const std::vector<std::string>& requiredExtensions,
                const std::vector<std::string>& enabledLayers,
                const QueueSources& queueSources) :
  _physicalDevice(physicalDevice),
  _handle(VK_NULL_HANDLE),
  _allocator(VK_NULL_HANDLE),
  _features(requiredFeatures)
  //_queuesByTypes{}
{
  try
  {
    _createHandle(requiredExtensions, enabledLayers, queueSources);
    _initVMA();
    _buildQueues(queueSources);
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

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = uint32_t(queueCreateInfos.size());
  createInfo.pEnabledFeatures = &_features;

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
  if(vmaCreateAllocator(&allocatorInfo, &_allocator) != VK_SUCCESS)
  {
    throw std::runtime_error("Device: Failed to create memory allocator.");
  }
}

void Device::_buildQueues(const QueueSources& queueSources)
{
  /*_drawQueueIndex = 0;
  std::unique_ptr<CommandQueue> newQueue(new CommandQueue(
                                                  *this,
                                                  uint32_t(_drawFamilyIndex),
                                                  _drawQueueIndex,
                                                  DRAW_POOL_SIZE));
  _drawQueue = newQueue.get();
  _queues.push_back(std::move(newQueue));

  _transferQueueIndex = 0;
  if(_transferFamilyIndex == _drawFamilyIndex) _transferQueueIndex++;
  newQueue.reset(new CommandQueue(*this,
                                  uint32_t(_transferFamilyIndex),
                                  _transferQueueIndex,
                                  TRANSFER_POOL_SIZE));
  _transferQueue = newQueue.get();
  _queues.push_back(std::move(newQueue));

  if(presentationEnabled)
  {
    if(_presentationFamilyIndex == _drawFamilyIndex)
    {
      _presentationQueueIndex = _drawQueueIndex;
      _presentationQueue = _drawQueue;
    }
    else
    {
      _presentationQueueIndex = 0;
      if(_presentationFamilyIndex == _transferFamilyIndex)
      {
        _presentationQueueIndex++;
      }
      newQueue.reset(new CommandQueue(*this,
                                      uint32_t(_presentationFamilyIndex),
                                      _presentationQueueIndex,
                                      DRAW_POOL_SIZE));
      _presentationQueue = newQueue.get();
      _queues.push_back(std::move(newQueue));
    }
  }
  else _presentationQueue = nullptr;*/
}

Device::~Device() noexcept
{
  _cleanup();
}

void Device::_cleanup() noexcept
{
  //_queuesByTypes = {};
  //_queues.clear();

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

bool Device::isSurfaceSuitable(const RenderSurface& surface) const
{
  /*if(!_physicalDevice.isSurfaceSuitable(surface)) return false;

  PhysicalDevice::QueuesInfo queuesInfo = _physicalDevice.queuesInfo();
  if(_presentationFamilyIndex == queuesInfo.size()) return false;

  return queuesInfo[_presentationFamilyIndex].isPresentSupported(surface);*/
  return false;
}
