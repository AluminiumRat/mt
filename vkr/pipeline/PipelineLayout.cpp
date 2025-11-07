#include <util/Abort.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/Device.h>

using namespace mt;

PipelineLayout::PipelineLayout(Device& device) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  try
  {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if(vkCreatePipelineLayout(device.handle(),
                              &pipelineLayoutInfo,
                              nullptr,
                              &_handle) != VK_SUCCESS)
    {
      Abort("PipelineLayout::PipelineLayout: unable to create layout handle");
    }
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

PipelineLayout::~PipelineLayout() noexcept
{
  _cleanup();
}

void PipelineLayout::_cleanup() noexcept
{
  if (_handle != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
