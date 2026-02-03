#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <hld/FrameBuildContext.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ColorFrameCommonSet::ColorFrameCommonSet(Device& device) :
  _device(device)
{
  _createLayouts();

  _uniformBuffer = new DataBuffer(_device,
                                  sizeof(Camera::ShaderData) +
                                    sizeof(GlobalLight::UniformBufferData),
                                  DataBuffer::UNIFORM_BUFFER,
                                  "ColorFrameCommonData");
}

void ColorFrameCommonSet::_createLayouts()
{
  static constexpr VkShaderStageFlags allStages = VK_SHADER_STAGE_ALL_GRAPHICS |
                                                  VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutBinding commonSetBindings[1];
  commonSetBindings[0] = {};
  commonSetBindings[0].binding = uniformBufferBinding;
  commonSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  commonSetBindings[0].descriptorCount = 1;
  commonSetBindings[0].stageFlags = allStages;

  _setLayout = new DescriptorSetLayout(_device, commonSetBindings);

  _pipelineLayout = new PipelineLayout( _device,
                                        std::span(&_setLayout, 1));
}

void ColorFrameCommonSet::update( CommandProducerGraphic& commandProducer,
                                  const FrameBuildContext& frameContext,
                                  const GlobalLight& illumination)
{
  _updateuniformBuffer(commandProducer, frameContext, illumination);

  if(_descriptorSet == nullptr)
  {
    Ref<DescriptorPool> pool(new DescriptorPool(_device,
                                                _setLayout->descriptorCounter(),
                                                1,
                                                DescriptorPool::STATIC_POOL));
    _descriptorSet = pool->allocateSet(*_setLayout);

    _descriptorSet->attachUniformBuffer(*_uniformBuffer, uniformBufferBinding);
    _descriptorSet->finalize();
  }
}

void ColorFrameCommonSet::_updateuniformBuffer(
                                        CommandProducerGraphic& commandProducer,
                                        const FrameBuildContext& frameContext,
                                        const GlobalLight& illumination)
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

  GlobalLight::UniformBufferData globalLightData = illumination.uniformData();
  uploadedData = commandProducer.uniformMemorySession().write(globalLightData);
  commandProducer.copyFromBufferToBuffer( *uploadedData.buffer,
                                          *_uniformBuffer,
                                          uploadedData.offset,
                                          uniformBufferCursor,
                                          sizeof(globalLightData));
  uniformBufferCursor += sizeof(globalLightData);
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
