#include <util/Abort.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/Device.h>

using namespace mt;

DescriptorSetLayout::DescriptorSetLayout(Device& device) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  try
  {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    if(vkCreateDescriptorSetLayout( device.handle(),
                                    &layoutInfo,
                                    nullptr,
                                    &_handle) != VK_SUCCESS)
    {
      Abort("DescriptorSetLayout: failed to create descriptor set layout");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

DescriptorSetLayout::~DescriptorSetLayout() noexcept
{
  _cleanup();
}

void DescriptorSetLayout::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}