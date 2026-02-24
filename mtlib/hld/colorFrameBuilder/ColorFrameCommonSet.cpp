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

  _commonLinearSampler = new Sampler( _device,
                                      VK_FILTER_LINEAR,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      0,
                                      true,
                                      4.0f);
}

void ColorFrameCommonSet::_createLayouts()
{
  _setLayout = new DescriptorSetLayout(_device, bindings);
  _pipelineLayout = new PipelineLayout( _device, std::span(&_setLayout, 1));
}

void ColorFrameCommonSet::update( CommandProducerGraphic& commandProducer,
                                  const FrameBuildContext& frameContext,
                                  const EnvironmentScene& environment)
{
  _updateuniformBuffer(commandProducer, frameContext, environment);

  Ref<DescriptorPool> pool(new DescriptorPool(_device,
                                              _setLayout->descriptorCounter(),
                                              1,
                                              DescriptorPool::STATIC_POOL));
  _descriptorSet = pool->allocateSet(*_setLayout);

  _descriptorSet->attachUniformBuffer(*_uniformBuffer, uniformBufferBinding);
  _descriptorSet->attachSampledImage( *_iblLut,
                                      iblLutBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampler(*_iblLutSampler, iblLutSamplerBinding);
  _descriptorSet->attachSampledImage( environment.irradianceMap(),
                                      iblIrradianceMapBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampledImage( environment.specularMap(),
                                      iblspecularMapBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampler(*_commonLinearSampler,
                                commonLinearSamplerBinding);
  _descriptorSet->finalize();
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
