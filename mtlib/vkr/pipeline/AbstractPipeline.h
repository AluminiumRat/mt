#pragma once

#include <span>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>
#include <vkr/pipeline/PipelineLayout.h>

namespace mt
{
  class Device;
  class ShaderModule;

  //  Базовый класс для GraphicPipeline и ComputePipeline
  //  Обертка вокруг VkPipeline, общий код для графического и компьют конвеера
  class AbstractPipeline : public RefCounter
  {
  public:
    enum Type
    {
      COMPUTE_PIPELINE,
      GRAPHIC_PIPELINE
    };

    // Информация о шейдерных стадиях пайплайна
    struct ShaderInfo
    {
      const ShaderModule* module;     // Не должен быть nullptr
      VkShaderStageFlagBits stage;    // Должен указывать строго на 1 стадию
      std::string entryPoint;
    };

  public:
    AbstractPipeline(Type type, const PipelineLayout& layout);
    AbstractPipeline(const AbstractPipeline&) = delete;
    AbstractPipeline& operator = (const AbstractPipeline&) = delete;
    virtual ~AbstractPipeline() noexcept;
  public:

    inline Device& device() const noexcept;
    inline Type type() const noexcept;
    inline VkPipeline handle() const noexcept;
    inline const PipelineLayout& layout() const noexcept;

  protected:
    //  Должен вызываться ровно 1 раз в конструкторе дочернего класса
    void setHandle(VkPipeline handle);

    using VkShadersInfo = std::vector<VkPipelineShaderStageCreateInfo>;
    static VkShadersInfo createVkShadersInfo(
                                          std::span<const ShaderInfo> shaders);
    virtual void cleanup() noexcept;

  private:
    Device& _device;
    Type _type;
    VkPipeline _handle;
    ConstRef<PipelineLayout> _layout;
  };

  inline Device& AbstractPipeline::device() const noexcept
  {
    return _device;
  }

  inline AbstractPipeline::Type AbstractPipeline::type() const noexcept
  {
    return _type;
  }

  inline VkPipeline AbstractPipeline::handle() const noexcept
  {
    return _handle;
  }

  inline const PipelineLayout& AbstractPipeline::layout() const noexcept
  {
    return *_layout;
  }
}