#include <stdexcept>

#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/FrameBuildContext.h>
#include <resourceManagement/TextureManager.h>
#include <technique/DescriptorSetType.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ColorFrameCommonSet::ColorFrameCommonSet( Device& device,
                                          TextureManager& textureManager) :
  _device(device),
  _textureManager(textureManager),
  _frameIndex(0)
{
  _createLayouts();

  _uniformBuffer = new DataBuffer(_device,
                                  sizeof(UniformCommonData),
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
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      0,
                                      true,
                                      4.0f);

  _commonNearestSampler = new Sampler(_device,
                                      VK_FILTER_NEAREST,
                                      VK_FILTER_NEAREST,
                                      VK_SAMPLER_MIPMAP_MODE_NEAREST);
}

void ColorFrameCommonSet::_createLayouts()
{
  _setLayout = new DescriptorSetLayout(_device, bindings);
  _pipelineLayout = new PipelineLayout( _device, std::span(&_setLayout, 1));
}

void ColorFrameCommonSet::update( CommandProducerGraphic& commandProducer,
                                  const FrameBuildContext& frameContext,
                                  const EnvironmentScene& environment,
                                  const ImageView& linearDepthHalfBuffer,
                                  const ImageView& normalHalfBuffer,
                                  const ImageView& hiZBuffer,
                                  const ImageView& reprojectionBuffer,
                                  const ImageView& shadowBuffer)
{
  //  Апдэйтим информацию о камере
  if(_frameIndex == 0)
  {
    _currentCameraData = frameContext.viewCamera->makeShaderData();
    _prevCameraData = _currentCameraData;
  }
  else
  {
    _prevCameraData = _currentCameraData;
    _currentCameraData = frameContext.viewCamera->makeShaderData();
  }

  _updateuniformBuffer( commandProducer,
                        frameContext,
                        environment,
                        glm::uvec2(linearDepthHalfBuffer.extent()),
                        glm::uvec2(hiZBuffer.extent()));

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
  _descriptorSet->attachSampledImage( linearDepthHalfBuffer,
                                      linearDepthHalfBufferBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampledImage( normalHalfBuffer,
                                      normalHalfBufferBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampledImage( hiZBuffer,
                                      hiZBufferBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampledImage( reprojectionBuffer,
                                      reprojectionBufferBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampledImage( shadowBuffer,
                                      shadowBufferBinding,
                                      VK_SHADER_STAGE_ALL);
  _descriptorSet->attachSampler(*_commonLinearSampler,
                                commonLinearSamplerBinding);
  _descriptorSet->attachSampler(*_commonNearestSampler,
                                commonNearestSamplerBinding);
  _descriptorSet->finalize();

  _frameIndex++;
}

void ColorFrameCommonSet::_updateuniformBuffer(
                                        CommandProducerGraphic& commandProducer,
                                        const FrameBuildContext& frameContext,
                                        const EnvironmentScene& environment,
                                        glm::uvec2 halfFrameExtent,
                                        glm::uvec2 hiZExtent)
{
  UniformCommonData uniformData{};
  uniformData.cameraData = _currentCameraData;
  uniformData.prevCameraData = _prevCameraData;

  uniformData.environment = environment.uniformData();

  uniformData.frameExtent = glm::vec4((float)frameContext.frameExtent.x,
                                      (float)frameContext.frameExtent.y,
                                      1.0f / frameContext.frameExtent.x,
                                      1.0f / frameContext.frameExtent.y);
  uniformData.iFrameExtent = frameContext.frameExtent;
  uniformData.halfExtent = glm::vec4((float)halfFrameExtent.x,
                                      (float)halfFrameExtent.y,
                                      1.0f / halfFrameExtent.x,
                                      1.0f / halfFrameExtent.y);
  uniformData.iHalfExtent = halfFrameExtent;
  uniformData.hiZExtent = glm::vec4(  (float)hiZExtent.x,
                                      (float)hiZExtent.y,
                                      1.0f / hiZExtent.x,
                                      1.0f / hiZExtent.y);
  uniformData.iHiZExtent = hiZExtent;
  uniformData.frameIndex = _frameIndex;

  commandProducer.uploadToBuffer( *_uniformBuffer,
                                  0,
                                  sizeof(uniformData),
                                  &uniformData);
}

void ColorFrameCommonSet::bind(CommandProducerGraphic& commandProducer) const
{
  MT_ASSERT(_descriptorSet != nullptr);
  commandProducer.bindDescriptorSetGraphic( *_descriptorSet,
                                            (uint32_t)DescriptorSetType::COMMON,
                                            *_pipelineLayout);
  commandProducer.bindDescriptorSetCompute( *_descriptorSet,
                                            (uint32_t)DescriptorSetType::COMMON,
                                            *_pipelineLayout);
}

void ColorFrameCommonSet::unbind(
                        CommandProducerGraphic& commandProducer) const noexcept
{
  commandProducer.unbindDescriptorSetCompute(
                                          (uint32_t)DescriptorSetType::COMMON);
  commandProducer.unbindDescriptorSetGraphic(
                                          (uint32_t)DescriptorSetType::COMMON);
}
