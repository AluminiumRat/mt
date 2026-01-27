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
  _commonSet(device),
  _opaqueColorStage(device)
{
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
    std::unique_ptr<CommandProducerGraphic> prepareProducer =
                                        _device.graphicQueue()->startCommands();
    _updateBuffers(target);
    _commonSet.update(*prepareProducer, frameContext, illumination);
    _device.graphicQueue()->submitCommands(std::move(prepareProducer));
  }

  {
    //  Opaque проход
    std::unique_ptr<CommandProducerGraphic> opaqueProducer =
                                  _device.graphicQueue()->startCommands(
                                                  OpaqueColorStage::stageName);
    {
      ColorFrameCommonSet::Bind(_commonSet, *opaqueProducer);
      _opaqueColorStage.draw( *opaqueProducer,
                              _drawPlan,
                              frameContext);
    }
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
