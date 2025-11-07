#pragma once

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;

  //  Обертка вокруг VkPipelineLayout
  //  Информация для пайплайна, какие ресурсы на каких стадиях он должен
  //  принимать. Используется при создании GraphicPipeline и ComputePipeline
  class PipelineLayout : public RefCounter
  {
  public:
    PipelineLayout(Device& device);
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
  };

  inline VkPipelineLayout PipelineLayout::handle() const noexcept
  {
    return _handle;
  }
}