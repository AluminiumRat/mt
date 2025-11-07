#pragma once

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;

  //  Обертка вокруг VkDescriptorSetLayout
  //  Описывает, какие типы ресурсов содержатся в отдельном наборе ресурсов (
  //    дескриптор сете).
  //  Используется при создании пайплайнов.
  class DescriptorSetLayout : public RefCounter
  {
  public:
    DescriptorSetLayout(Device& device);
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator = (const DescriptorSetLayout&) = delete;
  protected:
    virtual ~DescriptorSetLayout() noexcept;
  public:

    inline VkDescriptorSetLayout handle() const noexcept;

  private:
    void _cleanup() noexcept;

  private:
    Device& _device;
    VkDescriptorSetLayout _handle;
  };

  inline VkDescriptorSetLayout DescriptorSetLayout::handle() const noexcept
  {
    return _handle;
  }
}