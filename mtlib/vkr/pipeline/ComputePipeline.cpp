#include <stdexcept>

#include <util/Assert.h>
#include <vkr/pipeline/ComputePipeline.h>
#include <vkr/Device.h>

using namespace mt;

ComputePipeline::ComputePipeline( std::span<const ShaderInfo> shaders,
                                  const PipelineLayout& layout) :
  AbstractPipeline(COMPUTE_PIPELINE, layout)
{
  VkShadersInfo vkShadersInfo = createVkShadersInfo(shaders);
  MT_ASSERT(vkShadersInfo.size() == 1);

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = vkShadersInfo[0];
  pipelineInfo.layout = layout.handle();

  VkPipeline handle = VK_NULL_HANDLE;
  if(vkCreateComputePipelines(device().handle(),
                              VK_NULL_HANDLE,
                              1,
                              &pipelineInfo,
                              nullptr,
                              &handle) != VK_SUCCESS)
  {
    throw std::runtime_error("ComputePipeline: Unable to create pipeline");
  }
  setHandle(handle);
}