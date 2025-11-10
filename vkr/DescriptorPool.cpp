#include <stdexcept>

#include <util/Assert.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/DescriptorPool.h>
#include <vkr/Device.h>

using namespace mt;

DescriptorPool::DescriptorPool( Device& device,
                                const DescriptorCounter& totalDescriptors,
                                uint32_t totalSets,
                                Mode mode) :
  _handle(VK_NULL_HANDLE),
  _device(device),
  _mode(mode),
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

VkDescriptorSet DescriptorPool::allocateVolatileSet(
                                            const DescriptorSetLayout& layout)
{
  MT_ASSERT(_mode == VOLATILE_POOL);
  MT_ASSERT(_descriptorsLeft.contains(layout.descriptorCounter()))
  MT_ASSERT(_setsLeft != 0);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = _handle;
  allocInfo.descriptorSetCount = 1;
  VkDescriptorSetLayout layoutHandle = layout.handle();
  allocInfo.pSetLayouts = &layoutHandle;
  VkDescriptorSet newSet = VK_NULL_HANDLE;
  if(vkAllocateDescriptorSets(_device.handle(),
                              &allocInfo,
                              &newSet) != VK_SUCCESS)
  {
    throw std::runtime_error("DescriptorPool: Failed to allocate uniform descriptor set.");
  }

  _descriptorsLeft.reduce(layout.descriptorCounter());
  _setsLeft--;

  return newSet;
}

void DescriptorPool::reset()
{
  MT_ASSERT(_mode == VOLATILE_POOL);

  if(vkResetDescriptorPool(_device.handle(), _handle, 0) != VK_SUCCESS)
  {
    throw std::runtime_error("DescriptorPool: Failed to reset volatile descriptor pool.");
  }
  _descriptorsLeft = _totalDescriptors;
  _setsLeft = _totalSets;
}
