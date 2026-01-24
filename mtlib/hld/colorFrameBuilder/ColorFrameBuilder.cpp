#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/ColorFrameContext.h>
#include <hld/colorFrameBuilder/GlobalLight.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/HLDLib.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <vkr/image/ImageView.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ColorFrameBuilder::ColorFrameBuilder(Device& device) :
  _device(device),
  _memoryPool(100 * 1024),
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
                              const ExtraDraw& imGuiDraw,
                              CommandProducerGraphic& commandProducer)
{
  _drawPlan.clear();
  scene.fillDrawPlan(_drawPlan, viewCamera, _frameTypeIndex);

  ColorFrameContext frameContext{};
  frameContext.frameTypeIndex = _frameTypeIndex;
  frameContext.drawPlan = &_drawPlan;
  frameContext.commandMemoryPool = &_memoryPool;
  frameContext.viewCamera = &viewCamera;
  frameContext.illumination = &illumination;

  Ref<DescriptorSet> commonSet;
  {
    //  Подготовительные работы
    /*std::unique_ptr<CommandProducerGraphic> initProducer =
                                        _device.graphicQueue()->startCommands();
    frameContext.commandProducer = initProducer.get();*/

    _updateBuffers(target);

    /*frameContext.commandProducer = nullptr;
    _device.graphicQueue()->submitCommands(std::move(initProducer));*/
  }

  {
    //  Opaque проход
    std::unique_ptr<CommandProducerGraphic> opaqueProducer =
                                        _device.graphicQueue()->startCommands();
    frameContext.commandProducer = opaqueProducer.get();

    commonSet = _buildCommonSet(frameContext);

    _opaqueColorStage.draw(frameContext, *commonSet, *_commonSetPipelineLayout);

    frameContext.commandProducer = nullptr;
    _device.graphicQueue()->submitCommands(std::move(opaqueProducer));
  }

  {
    //  Сборка конечного кадра
    frameContext.commandProducer = &commandProducer;

    commandProducer.beginDebugLabel("LDRStage");
      CommandProducerGraphic::RenderPass renderPass(commandProducer, target);
        if(imGuiDraw)
        {
          commandProducer.beginDebugLabel("ImGui");
          imGuiDraw(commandProducer);
          frameContext.commandProducer->endDebugLabel();
        }
      renderPass.endPass();
    frameContext.commandProducer->endDebugLabel();

    frameContext.commandProducer = nullptr;
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
                                                    ColorFrameContext& context)
{
  Camera::ShaderData cameraData = context.viewCamera->makeShaderData();
  UniformMemoryPool::MemoryInfo uploadedCameraData =
              context.commandProducer->uniformMemorySession().write(cameraData);
  context.commandProducer->copyFromBufferToBuffer(*uploadedCameraData.buffer,
                                                  *_cameraBuffer,
                                                  uploadedCameraData.offset,
                                                  0,
                                                  sizeof(Camera::ShaderData));

  Ref<DescriptorSet> commonDescriptorSet =
      context.commandProducer->descriptorPool().allocateSet(*_commonSetLayout);
  commonDescriptorSet->attachUniformBuffer(*_cameraBuffer, cameraBufferBinding);
  commonDescriptorSet->attachUniformBuffer(
                                          context.illumination->uniformBuffer(),
                                          illuminationBufferBinding);
  commonDescriptorSet->finalize();

  return commonDescriptorSet;
}
