#pragma once

#include <span>

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

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

    inline VkDescriptorSetLayout handle() const noexcept;

  private:
    Device& _device;
    VkDescriptorSetLayout _handle;
  };

  inline VkDescriptorSetLayout DescriptorSetLayout::handle() const noexcept
  {
    return _handle;
  }
}