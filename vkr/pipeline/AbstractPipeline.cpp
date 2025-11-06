#include <util/Assert.h>
#include <vkr/pipeline/AbstractPipeline.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/Device.h>

using namespace mt;

AbstractPipeline::AbstractPipeline(Device& device, VkPipeline handle) :
  _device(device),
  _handle(handle)
{
  MT_ASSERT(_handle != VK_NULL_HANDLE);
}

AbstractPipeline::VkShadersInfo AbstractPipeline::createVkShadersInfo(
                                            std::span<const ShaderInfo> shaders)
{
  MT_ASSERT(!shaders.empty());

  std::vector<VkPipelineShaderStageCreateInfo> vkShadersInfo;
  vkShadersInfo.reserve(shaders.size());

  for(const ShaderInfo& shader : shaders)
  {
    MT_ASSERT(shader.module != nullptr);
    MT_ASSERT(!shader.entryPoint.empty());

    VkPipelineShaderStageCreateInfo vkShaderInfo{};
    vkShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkShaderInfo.stage = shader.stage;
    vkShaderInfo.module = shader.module->handle();
    vkShaderInfo.pName = shader.entryPoint.c_str();
  }

  return vkShadersInfo;
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
