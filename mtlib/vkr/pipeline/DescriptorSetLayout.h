#pragma once

#include <span>

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

#include <vkr/pipeline/DescriptorCounter.h>

namespace mt
{
  class Device;

  //  RAII обертка вокруг VkDescriptorSetLayout
  //  Описывает, какие типы ресурсов содержатся в отдельном наборе ресурсов (
  //    дескриптор сете).
  //  Используется при создании пайплайнов.
  class DescriptorSetLayout : public RefCounter
  {
  public:
    DescriptorSetLayout(Device& device,
                        std::span<const VkDescriptorSetLayoutBinding> bindings);
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator = (const DescriptorSetLayout&) = delete;
  protected:
    virtual ~DescriptorSetLayout() noexcept;
  public:

    inline Device& device() const noexcept;
    inline VkDescriptorSetLayout handle() const noexcept;
    inline const DescriptorCounter& descriptorCounter() const noexcept;

  private:
    Device& _device;
    VkDescriptorSetLayout _handle;
    DescriptorCounter _descriptorCounter;
  };

  inline Device& DescriptorSetLayout::device() const noexcept
  {
    return _device;
  }

  inline VkDescriptorSetLayout DescriptorSetLayout::handle() const noexcept
  {
    return _handle;
  }

  inline const DescriptorCounter&
                        DescriptorSetLayout::descriptorCounter() const noexcept
  {
    return _descriptorCounter;
  }
}