#pragma once

#include <span>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;
  class ShaderModule;

  //  Базовый класс для GraphicPipeline и ComputePipeline
  //  Обертка вокруг VkPipeline, общий код для графического и компьют конвеера
  class AbstractPipeline : public RefCounter
  {
  public:
    // Информация о шейдерных стадиях пайплайна
    struct ShaderInfo
    {
      const ShaderModule* module;     // Не должен быть nullptr
      VkShaderStageFlagBits stage;    // Должен указывать строго на 1 стадию
      std::string entryPoint;
    };

  public:
    AbstractPipeline(Device& device);
    AbstractPipeline(const AbstractPipeline&) = delete;
    AbstractPipeline& operator = (const AbstractPipeline&) = delete;
    virtual ~AbstractPipeline() noexcept;
  public:

    inline VkPipeline handle() const noexcept;

  protected:
    //  Должен вызываться ровно 1 раз в конструкторе дочернего класса
    void setHandle(VkPipeline handle);

    using VkShadersInfo = std::vector<VkPipelineShaderStageCreateInfo>;
    static VkShadersInfo createVkShadersInfo(
                                          std::span<const ShaderInfo> shaders);
    virtual void cleanup() noexcept;

  private:
    Device& _device;
    VkPipeline _handle;
  };

  inline VkPipeline AbstractPipeline::handle() const noexcept
  {
    return _handle;
  }
}