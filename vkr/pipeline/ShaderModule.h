#pragma once

#include <memory>
#include <span>
#include <string>

#include <vulkan/vulkan.h>

namespace mt
{
  class Device;

  //  Обертка вокруг VkShaderModule
  //  Представляет собой отдельный скомпилированный шейдерный модуль
  class ShaderModule
  {
  public:
    //  Интерфейс загрузчика данных для шейдера. Используйте
    //  ShaderModule::setShaderLoader и getShaderLoader для установки/получения
    //  текущего используемого лоадера
    class ShaderLoader
    {
    public:
      virtual ~ShaderLoader() noexcept = default;
      virtual std::vector<uint32_t> loadShader(const char* filename) = 0;
    };

  public:
    // Создать шейдер из данных SPIR-V в памяти
    ShaderModule( Device& device,
                  std::span<const uint32_t> spvData,
                  const char* debugName);
    // Загрузить шейдер в формате SPIR-V из файла
    ShaderModule(Device& device, const char* spvFilename);
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator = (const ShaderModule&) = delete;
    virtual ~ShaderModule() noexcept;

    inline VkShaderModule handle() const noexcept;
    inline const std::string& debugName() const noexcept;

    //  Установить загрузчик данных для шейдеров
    //  Если передать nullptr, то будет создан дефолтный загрузчик
    static void setShaderLoader(std::unique_ptr<ShaderLoader> newLoader);
    static ShaderLoader& getShaderLoader() noexcept;

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