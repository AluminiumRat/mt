#include <vector>

#include <util/Abort.h>
#include <util/Assert.h>
#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/Device.h>

using namespace mt;

PipelineLayout::PipelineLayout(
                        Device& device,
                        std::span<const DescriptorSetLayout*> descriptorSets) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  try
  {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Обходим лэйауты сетов и формируем массив хэндлов, за одним лочим
    //  от преждевременного удаления
    std::vector<VkDescriptorSetLayout> handlesArray;
    if(descriptorSets.size() != 0)
    {
      handlesArray.reserve(descriptorSets.size());
      _setLayouts.reserve(descriptorSets.size());
      for(const DescriptorSetLayout* setLayout : descriptorSets)
      {
        MT_ASSERT(setLayout != nullptr);
        MT_ASSERT(&setLayout->device() == &_device);
        handlesArray.push_back(setLayout->handle());
        _setLayouts.push_back(ConstRef(setLayout));
        _descriptorCounter.add(setLayout->descriptorCounter());
      }

      pipelineLayoutInfo.setLayoutCount = uint32_t(handlesArray.size());
      pipelineLayoutInfo.pSetLayouts = handlesArray.data();
    }

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

PipelineLayout::PipelineLayout(
                      Device& device,
                      std::span<ConstRef<DescriptorSetLayout>> descriptorSets):
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  try
  {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Обходим лэйауты сетов и формируем массив хэндлов, за одним лочим
    //  от преждевременного удаления
    std::vector<VkDescriptorSetLayout> handlesArray;
    if(descriptorSets.size() != 0)
    {
      handlesArray.reserve(descriptorSets.size());
      _setLayouts.reserve(descriptorSets.size());
      for(ConstRef<DescriptorSetLayout>& setLayout : descriptorSets)
      {
        MT_ASSERT(setLayout != nullptr);
        MT_ASSERT(&setLayout->device() == &_device);
        handlesArray.push_back(setLayout->handle());
        _setLayouts.push_back(ConstRef(setLayout));
        _descriptorCounter.add(setLayout->descriptorCounter());
      }

      pipelineLayoutInfo.setLayoutCount = uint32_t(handlesArray.size());
      pipelineLayoutInfo.pSetLayouts = handlesArray.data();
    }

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
