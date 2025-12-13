#pragma once

#include <filesystem>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  // Интеграция shaderC
  class ShaderCompilator
  {
  public:
    struct Define
    {
      const char* name;
      const char* value;
    };

    static std::vector<char> compile( const std::filesystem::path& file,
                                      VkShaderStageFlagBits shaderStage,
                                      std::span<const Define> defines);
  };
}