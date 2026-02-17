#include <stdexcept>

#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <hld/FrameBuildContext.h>
#include <resourceManagement/TextureManager.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

VkPipelineStageFlags ALL_SHADER_BITS =
                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
                            VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT |
                            VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;

ColorFrameCommonSet::ColorFrameCommonSet( Device& device,
                                          TextureManager& textureManager) :
  _device(device),
  _textureManager(textureManager)
{
  _createLayouts();

  _uniformBuffer = new DataBuffer(_device,
                                  sizeof(Camera::ShaderData) +
                                    sizeof(EnvironmentScene::UniformBufferData),
                                  DataBuffer::UNIFORM_BUFFER,
                                  "ColorFrameCommonData");

  _iblLut = _textureManager.loadImmediately("util/iblLut.dds",
                                            *_device.graphicQueue(),
                                            false)->image();
  if(_iblLut == nullptr) throw std::runtime_error("ColorFrameCommonSet: unable to load 'util/iblLut.dds'");

  _iblLutSampler = new Sampler( _device,
                                VK_FILTER_LINEAR,
                                VK_FILTER_LINEAR,
                                VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void ColorFrameCommonSet::_createLayouts()
{
  static constexpr VkShaderStageFlags allStages = VK_SHADER_STAGE_ALL_GRAPHICS |
                                                  VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutBinding commonSetBindings[3];
  commonSetBindings[0] = {};
  commonSetBindings[0].binding = uniformBufferBinding;
  commonSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  commonSetBindings[0].descriptorCount = 1;
  commonSetBindings[0].stageFlags = allStages;

  commonSetBindings[1] = {};
  commonSetBindings[1].binding = iblLutBinding;
  commonSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  commonSetBindings[1].descriptorCount = 1;
  commonSetBindings[1].stageFlags = allStages;

  commonSetBindings[2] = {};
  commonSetBindings[2].binding = iblLutSamplerBinding;
  commonSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  commonSetBindings[2].descriptorCount = 1;
  commonSetBindings[2].stageFlags = allStages;

  _setLayout = new DescriptorSetLayout(_device, commonSetBindings);

  _pipelineLayout = new PipelineLayout( _device,
                                        std::span(&_setLayout, 1));
}

void ColorFrameCommonSet::update( CommandProducerGraphic& commandProducer,
                                  const FrameBuildContext& frameContext,
                                  const EnvironmentScene& environment)
{
  _updateuniformBuffer(commandProducer, frameContext, environment);

  if(_descriptorSet == nullptr)
  {
    Ref<DescriptorPool> pool(new DescriptorPool(_device,
                                                _setLayout->descriptorCounter(),
                                                1,
                                                DescriptorPool::STATIC_POOL));
    _descriptorSet = pool->allocateSet(*_setLayout);

    _descriptorSet->attachUniformBuffer(*_uniformBuffer, uniformBufferBinding);
    _descriptorSet->attachSampledImage(*_iblLut, iblLutBinding, ALL_SHADER_BITS);
    _descriptorSet->attachSampler(*_iblLutSampler, iblLutSamplerBinding);
    _descriptorSet->finalize();
  }
}

void ColorFrameCommonSet::_updateuniformBuffer(
                                        CommandProducerGraphic& commandProducer,
                                        const FrameBuildContext& frameContext,
                                        const EnvironmentScene& environment)
{
  size_t uniformBufferCursor = 0;

  Camera::ShaderData cameraData = frameContext.viewCamera->makeShaderData();
  UniformMemoryPool::MemoryInfo uploadedData =
                      commandProducer.uniformMemorySession().write(cameraData);
  commandProducer.copyFromBufferToBuffer( *uploadedData.buffer,
                                          *_uniformBuffer,
                                          uploadedData.offset,
                                          uniformBufferCursor,
                                          sizeof(cameraData));
  uniformBufferCursor += sizeof(cameraData);

  EnvironmentScene::UniformBufferData environmentData = environment.uniformData();
  uploadedData = commandProducer.uniformMemorySession().write(environmentData);
  commandProducer.copyFromBufferToBuffer( *uploadedData.buffer,
                                          *_uniformBuffer,
                                          uploadedData.offset,
                                          uniformBufferCursor,
                                          sizeof(environmentData));
  uniformBufferCursor += sizeof(environmentData);
}

void ColorFrameCommonSet::bind(CommandProducerGraphic& commandProducer) const
{
  MT_ASSERT(_descriptorSet != nullptr);
  commandProducer.bindDescriptorSetGraphic( *_descriptorSet,
                                            (uint32_t)DescriptorSetType::COMMON,
                                            *_pipelineLayout);
}

void ColorFrameCommonSet::unbind(
                        CommandProducerGraphic& commandProducer) const noexcept
{
  commandProducer.unbindDescriptorSetGraphic(
                                          (uint32_t)DescriptorSetType::COMMON);
}
