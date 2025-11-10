#include <util/Assert.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/Device.h>

using namespace mt;

AbstractPipeline::AbstractPipeline(const PipelineLayout& layout) :
  _device(layout.device()),
  _handle(VK_NULL_HANDLE),
  _layout(&layout)
{
}

void AbstractPipeline::setHandle(VkPipeline handle)
{
  MT_ASSERT(handle != VK_NULL_HANDLE);
  _handle = handle;
}

AbstractPipeline::VkShadersInfo AbstractPipeline::createVkShadersInfo(
                                            std::span<const ShaderInfo> shaders)
{
  MT_ASSERT(!shaders.empty());

  std::vector<VkPipelineShaderStageCreateInfo> result;
  result.reserve(shaders.size());

  for(const ShaderInfo& shader : shaders)
  {
    MT_ASSERT(shader.module != nullptr);
    MT_ASSERT(!shader.entryPoint.empty());

    VkPipelineShaderStageCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = shader.stage;
    shaderInfo.module = shader.module->handle();
    shaderInfo.pName = shader.entryPoint.c_str();

    result.push_back(shaderInfo);
  }

  return result;
}

AbstractPipeline::~AbstractPipeline() noexcept
{
  cleanup();
}

void AbstractPipeline::cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
