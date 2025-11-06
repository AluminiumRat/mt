#pragma once

#include <span>
#include <string>

#include <vulkan/vulkan.h>

#include <vkr/RefCounter.h>

namespace mt
{
  class Device;

  //  Обертка вокруг VkShaderModule
  //  Представляет собой отдельный скомпилированный шейдерный модуль
  class ShaderModule : public RefCounter
  {
  public:
    // Создать шейдер из данных SPIR-V в памяти
    ShaderModule( Device& device,
                  std::span<const uint32_t> spvData,
                  const char* debugName);
    // Загрузить шейдер в формате SPIR-V из файла
    ShaderModule(Device& device, const char* spvFilename);
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator = (const ShaderModule&) = delete;
  protected:
    virtual ~ShaderModule() noexcept;
  public:

    inline VkShaderModule handle() const noexcept;
    inline const std::string& debugName() const noexcept;

  private:
    void _createHandle(std::span<const uint32_t> spvData);
    void _cleanup() noexcept;

  private:
    Device& _device;
    VkShaderModule _handle;
    std::string _debugName;
  };

  inline VkShaderModule ShaderModule::handle() const noexcept
  {
    return _handle;
  }

  inline const std::string& ShaderModule::debugName() const noexcept
  {
    return _debugName;
  }
}