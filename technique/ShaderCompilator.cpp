#include <stdexcept>
#include <mutex>

#include <shaderc/shaderc.hpp>

#include <technique/ShaderCompilator.h>
#include <util/Abort.h>
#include <vkr/pipeline/ShaderLoader.h>

using namespace mt;

shaderc::Compiler compiler;
std::mutex compilerMutex;

static shaderc_shader_kind getShaderKind(VkShaderStageFlagBits shaderStage)
{
  switch(shaderStage)
  {
  case VK_SHADER_STAGE_VERTEX_BIT:
    return shaderc_vertex_shader;
  case VK_SHADER_STAGE_FRAGMENT_BIT:
    return shaderc_fragment_shader;
  case VK_SHADER_STAGE_COMPUTE_BIT:
    return shaderc_compute_shader;
  case VK_SHADER_STAGE_GEOMETRY_BIT:
    return shaderc_geometry_shader;
  case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    return shaderc_tess_control_shader;
  case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    return shaderc_tess_evaluation_shader;
  case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
    return shaderc_raygen_shader;
  case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
    return shaderc_anyhit_shader;
  case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
    return shaderc_closesthit_shader;
  case VK_SHADER_STAGE_MISS_BIT_KHR:
    return shaderc_miss_shader;
  case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
    return shaderc_intersection_shader;
  case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
    return shaderc_callable_shader;
  default:
    Abort("ShaderCompilator::compile: unknown shader stage");
  }
}

std::vector<uint32_t> ShaderCompilator::compile(
                                              const char* filename,
                                              VkShaderStageFlagBits shaderStage,
                                              std::span<const Define> defines)
{
  std::string shaderText = ShaderLoader::getShaderLoader().loadText(filename);

  // Настраиваем макросы
  shaderc::CompileOptions options;
  options.AddMacroDefinition("M_PI", "3.1415926535897932384626433832795f");
  for (const Define& define : defines)
  {
    options.AddMacroDefinition(define.name, define.value);
  }

  //  Собираем SPIR-V код
  shaderc::SpvCompilationResult compilationResult;
  {
    std::lock_guard lock(compilerMutex);
    compilationResult = compiler.CompileGlslToSpv(shaderText,
                                                  getShaderKind(shaderStage),
                                                  filename,
                                                  options);
  }

  //  Разбираем результат компиляции
  if (compilationResult.GetCompilationStatus() !=
                                            shaderc_compilation_status_success)
  {
    throw std::runtime_error(std::string("Shader compilation error. File: ") + filename + "\n" + compilationResult.GetErrorMessage());
  }
  return std::vector<uint32_t>( compilationResult.cbegin(),
                                compilationResult.cend());
}