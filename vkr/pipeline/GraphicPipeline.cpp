#include <limits>

#include <util/Assert.h>

#include <vkr/pipeline/GraphicPipeline.h>
#include <vkr/FrameBufferFormat.h>
#include <vkr/Device.h>

using namespace mt;

// Маски для мультисэмплинга
static constexpr VkSampleMask allOne = std::numeric_limits<VkSampleMask>::max();
static VkSampleMask sampleMask[64] = { allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne, allOne };

GraphicPipeline::GraphicPipeline( 
              const FrameBufferFormat& frameBufferFormat,
              std::span<const ShaderInfo> shaders,
              VkPrimitiveTopology topology,
              const VkPipelineRasterizationStateCreateInfo& rasterizationState,
              const VkPipelineDepthStencilStateCreateInfo& depthStencilState,
              const VkPipelineColorBlendStateCreateInfo& blendingState,
              const PipelineLayout& layout) :
  AbstractPipeline(GRAPHIC_PIPELINE, layout)
{
  // Данные вершин будем грузить в вершинном шейдере через буферы
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = topology;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Вьюпорт и трафарет у нас всегда динамические, здесь просто заглушка
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  VkViewport viewport{.x =0,
                      .y = 0,
                      .width = 128,
                      .height = 128,
                      .minDepth = 0,
                      .maxDepth = 1};
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  VkRect2D scissor{.offset = {0, 0}, .extent = {128, 128}};
  viewportState.pScissors = &scissor;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};
  dynamicStateInfo.dynamicStateCount = 2;
  dynamicStateInfo.pDynamicStates = dynamicStates;

  VkPipelineMultisampleStateCreateInfo multisamplingState{};
  multisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingState.rasterizationSamples = frameBufferFormat.samplesCount();
  multisamplingState.pSampleMask = sampleMask;

  VkShadersInfo vkShadersInfo = createVkShadersInfo(shaders);
  MT_ASSERT(!vkShadersInfo.empty());

  VkPipelineRenderingCreateInfo pipelineRenderingInfo =
                                      frameBufferFormat.pipelineCreateInfo();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = uint32_t(vkShadersInfo.size());
  pipelineInfo.pStages = vkShadersInfo.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizationState;
  pipelineInfo.pMultisampleState = &multisamplingState;
  pipelineInfo.pDepthStencilState = &depthStencilState;
  pipelineInfo.pColorBlendState = &blendingState;
  pipelineInfo.pDynamicState = &dynamicStateInfo;

  pipelineInfo.layout = layout.handle();

  // Включаем dynamic rendering
  pipelineInfo.renderPass = VK_NULL_HANDLE;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.pNext = &pipelineRenderingInfo;

  VkPipeline handle = VK_NULL_HANDLE;
  if (vkCreateGraphicsPipelines(device().handle(),
                                VK_NULL_HANDLE,
                                1,
                                &pipelineInfo,
                                nullptr,
                                &handle) != VK_SUCCESS)
  {
    Abort("GraphicPipeline: Unable to create graphic pipeline");
  }
  setHandle(handle);
}
