#include <stdexcept>

#include <vkr/queue/TimelineSemaphore.h>
#include <vkr/Device.h>

using namespace mt;

TimelineSemaphore::TimelineSemaphore(uint64_t initialValue, Device& device) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  try
  {
    VkSemaphoreTypeCreateInfo timelineCreateInfo;
    timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineCreateInfo.pNext = NULL;
    timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue = initialValue;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = &timelineCreateInfo;
    if (vkCreateSemaphore(_device.handle(),
                          &semaphoreInfo,
                          nullptr,
                          &_handle) != VK_SUCCESS)
    {
      throw std::runtime_error("TimelineSemaphore: Failed to create semaphore.");
    }
  }
  catch(...)
  {
    _clear();
    throw;
  }
}

TimelineSemaphore::~TimelineSemaphore() noexcept
{
  _clear();
}

void TimelineSemaphore::_clear() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}

uint64_t TimelineSemaphore::getValue() const
{
  uint64_t value = 0;
  if(vkGetSemaphoreCounterValue(_device.handle(), _handle, &value)
                                                                != VK_SUCCESS)
  {
    throw std::runtime_error("TimelineSemaphore: Unable to get value.");
  }
  return value;
}

void TimelineSemaphore::waitFor(uint64_t value) const
{
  VkSemaphoreWaitInfo waitInfo;
  waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  waitInfo.pNext = NULL;
  waitInfo.flags = 0;
  waitInfo.semaphoreCount = 1;
  waitInfo.pSemaphores = &_handle;
  waitInfo.pValues = &value;

  if(vkWaitSemaphores(_device.handle(), &waitInfo, UINT64_MAX) != VK_SUCCESS)
  {
    throw std::runtime_error("TimelineSemaphore: Unable to wait for value.");
  }
}
