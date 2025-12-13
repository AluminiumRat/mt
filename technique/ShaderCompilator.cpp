#include <stdexcept>
#include <mutex>

#include <shaderc/shaderc.hpp>

#include <technique/ShaderCompilator.h>
#include <util/Abort.h>
#include <util/ContentLoader.h>

using namespace mt;

shaderc::Compiler compiler;
std::mutex compilerMutex;

//  Штука, которая подгружает и хранит include файлы для компилятора
class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
  virtual shaderc_include_result* GetInclude( const char* requested_source,
                                              shaderc_include_type type,
                                              const char* requesting_source,
                                              size_t include_depth) override;
  virtual void ReleaseInclude(shaderc_include_result* data) override;

private:
  //  Расширение, которое не только ссылается на данные, но ещё и хранит их
  struct IncludeInfo : public shaderc_include_result
  {
    std::string sourceNameStr;
    std::string contentStr;
  };
private:
  using Includes = std::vector<std::unique_ptr<IncludeInfo>>;
  Includes _includes;
};

shaderc_include_result* ShaderIncluder::GetInclude(
                                                  const char* requested_source,
                                                  shaderc_include_type type,
                                                  const char* requesting_source,
                                                  size_t include_depth)
{
  try
  {
    // Загружаем текст шейдера
    std::string shaderText;
    std::string errorString;
    try
    {
      std::filesystem::path filePatch((const char8_t*)requested_source);
      shaderText = ContentLoader::getLoader().loadText(filePatch);
      if (shaderText.empty()) throw std::runtime_error(std::string((const char*)filePatch.u8string().c_str()) + " : file is empty");
    }
    catch (std::exception& error)
    {
      errorString = error.what();
    }

    // Формируем результат для shaderC
    std::unique_ptr<IncludeInfo> include(new IncludeInfo{});
    if(!shaderText.empty())
    {
      include->sourceNameStr = requested_source;
      include->source_name = include->sourceNameStr.c_str();
      include->source_name_length = include->sourceNameStr.length();
      include->contentStr = std::move(shaderText);
      include->content = include->contentStr.c_str();
      include->content_length = include->contentStr.length();
    }
    else
    {
      include->source_name = nullptr;
      include->source_name_length = 0;
      include->contentStr = std::move(errorString);
      include->content = include->contentStr.c_str();
      include->content_length = include->contentStr.length();
    }

    IncludeInfo* includePtr = include.get();
    _includes.push_back(std::move(include));
    return includePtr;
  }
  catch(std::exception& error)
  {
    Log::error() << "ShaderIncluder::GetInclude: " << error.what();
    Abort("ShaderIncluder::GetInclude: unable to load include file");
  }
}

void ShaderIncluder::ReleaseInclude(shaderc_include_result* data)
{
  IncludeInfo* includePtr = static_cast<IncludeInfo*>(data);
  for(Includes::iterator iInclude = _includes.begin();
      iInclude != _includes.end();
      iInclude++)
  {
    if(iInclude->get() == includePtr)
    {
      _includes.erase(iInclude);
      return;
    }
  }
}

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

std::vector<char> ShaderCompilator::compile(const std::filesystem::path& file,
                                            VkShaderStageFlagBits shaderStage,
                                            std::span<const Define> defines)
{
  std::string shaderFilename = (const char*)file.u8string().c_str();
  std::string shaderText = ContentLoader::getLoader().loadText(file);
  if (shaderText.empty()) throw std::runtime_error(shaderFilename + " : file is empty");

  // Настраиваем макросы
  shaderc::CompileOptions options;
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  options.SetGenerateDebugInfo();
  options.SetWarningsAsErrors();
  options.SetAutoBindUniforms(true);
  options.SetAutoMapLocations(true);
  options.AddMacroDefinition("M_PI", "3.1415926535897932384626433832795f");
  options.AddMacroDefinition("STATIC", "0");
  options.AddMacroDefinition("VOLATILE", "1");
  options.AddMacroDefinition("COMMON", "2");
  for (const Define& define : defines)
  {
    options.AddMacroDefinition(define.name, define.value);
  }
  options.SetIncluder(std::make_unique<ShaderIncluder>());

  //  Собираем SPIR-V код
  shaderc::SpvCompilationResult compilationResult;
  {
    std::lock_guard lock(compilerMutex);
    compilationResult = compiler.CompileGlslToSpv(shaderText,
                                                  getShaderKind(shaderStage),
                                                  shaderFilename.c_str(),
                                                  options);
  }

  //  Разбираем результат компиляции
  if (compilationResult.GetCompilationStatus() !=
                                            shaderc_compilation_status_success)
  {
    throw std::runtime_error(std::string("Shader compilation error. File: ") + shaderFilename.c_str() + "\n" + compilationResult.GetErrorMessage());
  }
  return std::vector<char>( (const char*)compilationResult.cbegin(),
                            (const char*)compilationResult.cend());
}