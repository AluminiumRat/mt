#include <stdexcept>

#include <vkr/queue/QueueFamily.h>
#include <vkr/PhysicalDevice.h>
#include <vkr/WindowSurface.h>

using namespace mt;

QueueFamily::QueueFamily( PhysicalDevice& device,
                                  uint32_t familyIndex) :
  _device(&device),
  _index(familyIndex)
{
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( _device->handle(),
                                            &queueFamilyCount,
                                            nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties( _device->handle(),
                                            &queueFamilyCount,
                                            queueFamilies.data());
  _properties = queueFamilies[_index];
}

bool QueueFamily::isPresentSupported(const WindowSurface& surface) const
{
  VkBool32 presentSupport = false;
  if(vkGetPhysicalDeviceSurfaceSupportKHR(_device->handle(),
                                          _index,
                                          surface.handle(),
                                          &presentSupport) != VK_SUCCESS)
  {
    throw std::runtime_error("QueueFamily: Unable to get present support.");
  }
  return presentSupport;
}
