#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/pipeline/DescriptorSetLayout.h>
#include <vkr/RefCounter.h>
#include <vkr/Ref.h>

namespace mt
{
  class Device;

  //  Обертка вокруг VkPipelineLayout
  //  Информация для пайплайна, какие ресурсы на каких стадиях он должен
  //  принимать. Используется при создании GraphicPipeline и ComputePipeline
  class PipelineLayout : public RefCounter
  {
  public:
    // descriptorSets не должен содержать nullptr
    PipelineLayout( Device& device,
                    std::span<const DescriptorSetLayout*> descriptorSets);
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator = (const PipelineLayout&) = delete;
  protected:
    virtual ~PipelineLayout() noexcept;
  public:

    inline VkPipelineLayout handle() const noexcept;

  private:
    void _cleanup() noexcept;

  private:
    Device& _device;
    VkPipelineLayout _handle;

    std::vector<ConstRef<DescriptorSetLayout>> _setLayouts;
  };

  inline VkPipelineLayout PipelineLayout::handle() const noexcept
  {
    return _handle;
  }
}