#pragma once

#include <vulkan/vulkan.h>

#include <util/Bimap.h>
#include <vkr/pipeline/AbstractPipeline.h>

namespace mt
{
  extern const Bimap<AbstractPipeline::Type> pipelineMap;
  extern const Bimap<VkShaderStageFlagBits> shaderStageMap;
  extern const Bimap<VkSampleCountFlagBits> sampleCountMap;
  extern const Bimap<VkPrimitiveTopology> topologyMap;
  extern const Bimap<VkPolygonMode> polygonModeMap;
  extern const Bimap<VkCullModeFlagBits> cullModeMap;
  extern const Bimap<VkFrontFace> fronFaceMap;
  extern const Bimap<VkCompareOp> compareOpMap;
  extern const Bimap<VkStencilOp> stencilOpMap;
  extern const Bimap<VkBlendFactor> blendFactorMap;
  extern const Bimap<VkBlendOp> blendOpMap;
  extern const Bimap<VkColorComponentFlagBits> colorComponentMap;
  extern const Bimap<VkLogicOp> logicOpMap;
  extern const Bimap<VkFilter> filterMap;
  extern const Bimap<VkSamplerMipmapMode> mipmapModeMap;
  extern const Bimap<VkSamplerAddressMode> addressModeMap;
  extern const Bimap<VkBorderColor> borderColorMap;
}