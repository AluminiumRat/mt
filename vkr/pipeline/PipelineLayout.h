#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/pipeline/DescriptorCounter.h>
#include <vkr/pipeline/DescriptorSetLayout.h>

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
    // descriptorSets не должен содержать nullptr
    PipelineLayout( Device& device,
                    std::span<ConstRef<DescriptorSetLayout>> descriptorSets);
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator = (const PipelineLayout&) = delete;
  protected:
    virtual ~PipelineLayout() noexcept;
  public:

    inline Device& device() const noexcept;
    inline VkPipelineLayout handle() const noexcept;

    inline uint32_t setsNumber() const noexcept;
    inline const DescriptorSetLayout&
                                    setLayout(uint32_t setIndex) const noexcept;

    // Сумма дескриптеров для всех дескриптор сетов
    inline const DescriptorCounter& descriptorCounter() const noexcept;

  private:
    void _cleanup() noexcept;

  private:
    Device& _device;
    VkPipelineLayout _handle;

    std::vector<ConstRef<DescriptorSetLayout>> _setLayouts;
    DescriptorCounter _descriptorCounter;
  };

  inline Device& PipelineLayout::device() const noexcept
  {
    return _device;
  }

  inline VkPipelineLayout PipelineLayout::handle() const noexcept
  {
    return _handle;
  }

  inline uint32_t PipelineLayout::setsNumber() const noexcept
  {
    return uint32_t(_setLayouts.size());
  }

  inline const DescriptorSetLayout&
                    PipelineLayout::setLayout(uint32_t setIndex) const noexcept
  {
    return *_setLayouts[setIndex];
  }

  inline const DescriptorCounter&
                              PipelineLayout::descriptorCounter() const noexcept
  {
    return _descriptorCounter;
  }
}