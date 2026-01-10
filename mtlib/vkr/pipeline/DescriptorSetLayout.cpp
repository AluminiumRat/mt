#include <util/Abort.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/Device.h>

using namespace mt;

DescriptorSetLayout::DescriptorSetLayout(
                      Device& device,
                      std::span<const VkDescriptorSetLayoutBinding> bindings) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _descriptorCounter(DescriptorCounter::createFrom(bindings))
{
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

  if(!bindings.empty())
  {
    layoutInfo.bindingCount = uint32_t(bindings.size());
    layoutInfo.pBindings = bindings.data();
  }

  if(vkCreateDescriptorSetLayout( device.handle(),
                                  &layoutInfo,
                                  nullptr,
                                  &_handle) != VK_SUCCESS)
  {
    Abort("DescriptorSetLayout: failed to create descriptor set layout");
  }
}

DescriptorSetLayout::~DescriptorSetLayout() noexcept
{
  if (_handle != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
