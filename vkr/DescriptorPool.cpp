#include <stdexcept>

#include <util/Assert.h>
#include <vkr/DescriptorPool.h>
#include <vkr/Device.h>

using namespace mt;

DescriptorPool::DescriptorPool( const DescriptorCounter& totalDescriptors,
                                uint32_t totalSets,
                                Device& device) :
  _handle(VK_NULL_HANDLE),
  _device(device),
  _totalDescriptors(totalDescriptors),
  _descriptorsLeft(_totalDescriptors),
  _totalSets(totalSets),
  _setsLeft(totalSets)
{
  try
  {
    std::vector<VkDescriptorPoolSize> sizeInfo =
                                              _totalDescriptors.makeSizeInfo();
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = uint32_t(sizeInfo.size());
    poolInfo.pPoolSizes = sizeInfo.data();
    poolInfo.maxSets = _totalSets;
    if(vkCreateDescriptorPool(_device.handle(),
                              &poolInfo,
                              nullptr,
                              &_handle) != VK_SUCCESS)
    {
      throw std::runtime_error("DescriptorPool: Failed to create descriptor pool.");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

DescriptorPool::~DescriptorPool() noexcept
{
  _cleanup();
}

void DescriptorPool::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}

VkDescriptorSet DescriptorPool::allocateSet(
                                    VkDescriptorSetLayout layout,
                                    const DescriptorCounter& descriptorsNumber)
{
  MT_ASSERT(_descriptorsLeft.contains(descriptorsNumber))
  MT_ASSERT(_setsLeft != 0);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _handle;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;
  VkDescriptorSet newSet = VK_NULL_HANDLE;
  if(vkAllocateDescriptorSets(_device.handle(),
                              &allocInfo,
                              &newSet) != VK_SUCCESS)
  {
    throw std::runtime_error("DescriptorPool: Failed to allocate uniform descriptor set.");
  }

  _descriptorsLeft.reduce(descriptorsNumber);
  _setsLeft--;

  return newSet;
}

void DescriptorPool::reset()
{
  if(vkResetDescriptorPool(_device.handle(), _handle, 0) != VK_SUCCESS)
  {
    throw std::runtime_error("DescriptorPool: Failed to reset volatile descriptor pool.");
  }
  _descriptorsLeft = _totalDescriptors;
  _setsLeft = _totalSets;
}
