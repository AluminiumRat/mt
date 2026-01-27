#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/FrameBuildContext.h>
#include <hld/HLDLib.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <vkr/image/ImageView.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ColorFrameBuilder::ColorFrameBuilder(Device& device) :
  _device(device),
  _frameTypeIndex(HLDLib::instance().getFrameTypeIndex(frameTypeName)),
  _opaqueColorStage(device)
{
  VkDescriptorSetLayoutBinding commonSetBindings[2];
  commonSetBindings[cameraBufferBinding] = {};
  commonSetBindings[cameraBufferBinding].binding = 0;
  commonSetBindings[cameraBufferBinding].descriptorType =
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  commonSetBindings[cameraBufferBinding].descriptorCount = 1;
  commonSetBindings[cameraBufferBinding].stageFlags =
                    VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

  commonSetBindings[illuminationBufferBinding] = {};
  commonSetBindings[illuminationBufferBinding].binding = 1;
  commonSetBindings[illuminationBufferBinding].descriptorType =
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  commonSetBindings[illuminationBufferBinding].descriptorCount = 1;
  commonSetBindings[illuminationBufferBinding].stageFlags =
                    VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;

  _commonSetLayout = new DescriptorSetLayout(_device, commonSetBindings);

  _commonSetPipelineLayout = new PipelineLayout(_device,
                                                std::span(&_commonSetLayout,
                                                          1));
  _cameraBuffer = new DataBuffer( _device,
                                  sizeof(Camera::ShaderData),
                                  DataBuffer::UNIFORM_BUFFER,
                                  "CameraData");
}

void ColorFrameBuilder::draw( FrameBuffer& target,
                              const DrawScene& scene,
                              const Camera& viewCamera,
                              const GlobalLight& illumination,
                              const ExtraDraw& imGuiDraw)
{
  FrameBuildContext frameContext{};
  frameContext.frameType = _frameTypeIndex;
  frameContext.viewCamera = &viewCamera;
  frameContext.drawScene = &scene;

  _drawPlan.clear();
  scene.fillDrawPlan(_drawPlan, frameContext);

  {
    //  Подготовительные работы
    _updateBuffers(target);
  }

  {
    //  Opaque проход
    std::unique_ptr<CommandProducerGraphic> opaqueProducer =
                                  _device.graphicQueue()->startCommands(
                                                  OpaqueColorStage::stageName);

    Ref<DescriptorSet> commonSet = _buildCommonSet( *opaqueProducer,
                                                    viewCamera,
                                                    illumination);

    _opaqueColorStage.draw( *opaqueProducer,
                            _drawPlan,
                            frameContext,
                            *commonSet,
                            *_commonSetPipelineLayout);

    _device.graphicQueue()->submitCommands(std::move(opaqueProducer));
  }

  {
    //  Сборка конечного кадра
    std::unique_ptr<CommandProducerGraphic> finalizeProducer =
                              _device.graphicQueue()->startCommands("LDRStage");

    CommandProducerGraphic::RenderPass renderPass(*finalizeProducer, target);
      if(imGuiDraw)
      {
        finalizeProducer->beginDebugLabel("ImGui");
        imGuiDraw(*finalizeProducer);
        finalizeProducer->endDebugLabel();
      }
    renderPass.endPass();

    _device.graphicQueue()->submitCommands(std::move(finalizeProducer));
  }
}

void ColorFrameBuilder::_updateBuffers( FrameBuffer& targetFrameBuffer)
{
  if (_hdrBuffer == nullptr ||
      glm::uvec2(_hdrBuffer->extent()) != targetFrameBuffer.extent())
  {
    _hdrBuffer = new Image( _device,
                            VK_IMAGE_TYPE_2D,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT,
                            0,
                            hdrFormat,
                            glm::uvec3(targetFrameBuffer.extent(), 1),
                            VK_SAMPLE_COUNT_1_BIT,
                            1,
                            1,
                            false,
                            "HDRBuffer");
    _opaqueColorStage.setHdrBuffer(*_hdrBuffer);
  }

  if (_depthBuffer == nullptr ||
      glm::uvec2(_depthBuffer->extent()) != targetFrameBuffer.extent())
  {
    _depthBuffer = new Image( _device,
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              0,
                              depthFormat,
                              glm::uvec3(targetFrameBuffer.extent(), 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              1,
                              false,
                              "DepthBuffer");
    _opaqueColorStage.setDepthBuffer(*_depthBuffer);
  }
}

Ref<DescriptorSet> ColorFrameBuilder::_buildCommonSet(
                                        CommandProducerGraphic& commandProducer,
                                        const Camera& camera,
                                        const GlobalLight& illumination)
{
  Camera::ShaderData cameraData = camera.makeShaderData();
  UniformMemoryPool::MemoryInfo uploadedCameraData =
                      commandProducer.uniformMemorySession().write(cameraData);
  commandProducer.copyFromBufferToBuffer( *uploadedCameraData.buffer,
                                          *_cameraBuffer,
                                          uploadedCameraData.offset,
                                          0,
                                          sizeof(Camera::ShaderData));

  Ref<DescriptorSet> commonDescriptorSet =
                commandProducer.descriptorPool().allocateSet(*_commonSetLayout);
  commonDescriptorSet->attachUniformBuffer(*_cameraBuffer, cameraBufferBinding);
  commonDescriptorSet->attachUniformBuffer( illumination.uniformBuffer(),
                                            illuminationBufferBinding);
  commonDescriptorSet->finalize();

  return commonDescriptorSet;
}
