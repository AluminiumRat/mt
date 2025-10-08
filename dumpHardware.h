#pragma once

#include <bitset>

#include <util/util.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/VKRLib.h>
#include <vkr/WindowSurface.h>

// Дамп вулкан лэйеров
inline void dumpInstanceLayers()
{
  mt::VKRLib& vkrLib = mt::VKRLib::instance();
  mt::Log::info() << "Layers: [" << vkrLib.availableLayers().size() << "]";
  for(const VkLayerProperties& layer : vkrLib.availableLayers())
  {
    mt::Log::info() << "  " << layer.layerName;
  }
}

// Дамп расширений уровня инстанс
inline void dumpInstanceExtensions()
{
  mt::VKRLib& vkrLib = mt::VKRLib::instance();
  mt::Log::info() << "Instance extensions: [" << vkrLib.availableExtensions().size() << "]";
  for (const VkExtensionProperties& extension : vkrLib.availableExtensions())
  {
    mt::Log::info() << "  " << extension.extensionName;
  }
}

// Дам очередей
// testsurface нужен только для проверки совместимости, можно передавать nullptr
inline void dumpQueueFamilies(
  const mt::PhysicalDevice& device,
  const mt::WindowSurface* testsurface)
{
  mt::QueueFamiliesInfo queuesInfo = device.queuesInfo();
  mt::Log::info() << "  Queues families: [" << queuesInfo.size() << "]";
  for (int index = 0; index < queuesInfo.size(); index++)
  {
    mt::Log::info() << "    " << index;
    mt::Log::info() << "      Is graphic: " << queuesInfo[index].isGraphic();
    mt::Log::info() << "      Is compute: " << queuesInfo[index].isCompute();
    mt::Log::info() << "      Is transfer: " << queuesInfo[index].isTransfer();
    mt::Log::info() << "      Sparse binding: " << queuesInfo[index].sparseBindingSupported();
    if (testsurface != nullptr)
    {
      mt::Log::info() << "      Presentation support: " << queuesInfo[index].isPresentSupported(*testsurface);
    }
    mt::Log::info() << "      Available count: " << queuesInfo[index].queueCount();
  }
}

// Дамп расширений уровня устройства
inline void dumpDeviceExtensions(const mt::PhysicalDevice& device)
{
  mt::Log::info() << "  Device extensions: [" << device.availableExtensions().size() << "]";
  for (const VkExtensionProperties& extension : device.availableExtensions())
  {
    mt::Log::info() << "    " << extension.extensionName;
  }
}

// Какая память и сколько её есть на устройстве
inline void dumpDeviceMemory(const mt::PhysicalDevice& device)
{
  const mt::MemoryInfo& memoryInfo = device.memoryInfo();
  mt::Log::info() << "  Memory heaps: [" << memoryInfo.heaps.size() << "]";
  for (size_t heapIndex = 0; heapIndex < memoryInfo.heaps.size(); heapIndex++)
  {
    const mt::MemoryHeapInfo& heap = memoryInfo.heaps[heapIndex];
    mt::Log::info() << "    " << heapIndex;
    mt::Log::info() << "      Heap size: " << heap.size / (1024 * 1024) << "MB";
    mt::Log::info() << "      Device local: " << heap.isDeviceLocal();
    mt::Log::info() << "      Flags: " << std::bitset<32>(heap.flags);

    mt::Log::info() << "      Memory types: [" << heap.types.size() << "]";
    for (size_t index = 0; index < heap.types.size(); index++)
    {
      const mt::MemoryTypeInfo& type = *heap.types[index];
      mt::Log::info() << "        " << index;
      mt::Log::info() << "          Global index: " << type.index;
      mt::Log::info() << "          Device local: " << type.isDeviceLocal();
      mt::Log::info() << "          Host visible: " << type.isHostVisible();
      mt::Log::info() << "          Host coherent: " << type.isHostCoherent();
      mt::Log::info() << "          Device only: " << type.isDeviceOnly();
      mt::Log::info() << "          Flags: " << std::bitset<32>(type.flags);
    }
  }
}

// Дамп физических устройств
// testsurface нужен только для проверки совместимости, можно передавать nullptr
inline void dumpPhysicalDevices(
  bool dumpExtensions,
  bool dumpQueues,
  bool dumpMemory,
  const mt::WindowSurface* testsurface)
{
  mt::VKRLib& vkrLib = mt::VKRLib::instance();
  std::vector<mt::PhysicalDevice*> phisicalDevices = vkrLib.devices();
  mt::Log::info() << "Devices: [" << phisicalDevices.size() << "]";
  for (mt::PhysicalDevice* device : phisicalDevices)
  {
    mt::Log::info() << device->properties().deviceName;
    if(dumpQueues) dumpQueueFamilies(*device, testsurface);
    if(dumpExtensions) dumpDeviceExtensions(*device);
    if(dumpMemory) dumpDeviceMemory(*device);
  }
}

// Выводит в лог инфу об физических устройствах системы
// testsurface нужна только для того, чтобы выяснить кто может с ней
// взаимодействовать. Если передать nullptr, то совместимость не будет
// проверяться.
inline void dumpHardware(
  bool dumpLayers,
  bool dumpExtensions,
  bool dumpDevices,
  bool dumpQueues,
  bool dumpMemory,
  const mt::WindowSurface* testsurface)
{
  mt::VKRLib& vkrLib = mt::VKRLib::instance();
  mt::Log::info() << "---- HARDWARE DUMP ----";

  mt::Log::info() << "Validation support: " << vkrLib.isValidationSupported();
  mt::Log::info() << "Debug support: " << vkrLib.isDebugSupported();
  
  if(dumpLayers)
  {
    mt::Log::info();
    dumpInstanceLayers();
  }

  if(dumpExtensions)
  {
    mt::Log::info();
    dumpInstanceExtensions();
  }

  if(dumpDevices)
  {
    mt::Log::info();
    dumpPhysicalDevices(dumpExtensions, dumpQueues, dumpMemory, testsurface);
  }

  mt::Log::info() << "---- END HARDWARE DUMP ----";
}
