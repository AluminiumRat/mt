#include <gui/ImGuiRAII.h>
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
  _opaqueColorStage(device),
  _posteffects(device)
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
    _initBuffersLayout(*prepareProducer);
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

    _posteffectsPrepareLayouts(*finalizeProducer);
    _posteffects.prepare(*finalizeProducer, frameContext);

    _posteffectsResolveLayouts(*finalizeProducer);
    CommandProducerGraphic::RenderPass renderPass(*finalizeProducer, target);
      _posteffects.makeLDR(*finalizeProducer, frameContext);
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
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                            0,
                            hdrFormat,
                            glm::uvec3(targetFrameBuffer.extent(), 1),
                            VK_SAMPLE_COUNT_1_BIT,
                            1,
                            1,
                            false,
                            "HDRBuffer");
    _hdrBufferView = new ImageView( *_hdrBuffer,
                                    ImageSlice(*_hdrBuffer),
                                    VK_IMAGE_VIEW_TYPE_2D);
    _opaqueColorStage.setHdrBuffer(*_hdrBufferView);
    _posteffects.setHdrBuffer(*_hdrBufferView);
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
    _depthBufferView = new ImageView( *_depthBuffer,
                                      ImageSlice(*_depthBuffer),
                                      VK_IMAGE_VIEW_TYPE_2D);
    _opaqueColorStage.setDepthBuffer(*_depthBufferView);
  }
}

void ColorFrameBuilder::_initBuffersLayout(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_hdrBuffer,
                                ImageSlice(*_hdrBuffer),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                0,
                                0,
                                0,
                                0);

  commandProducer.imageBarrier(
                              *_depthBuffer,
                              ImageSlice(*_depthBuffer),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                              0,
                              0,
                              0,
                              0);
}

void ColorFrameBuilder::_posteffectsPrepareLayouts(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_hdrBuffer,
                                ImageSlice(*_hdrBuffer),
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT);
}

void ColorFrameBuilder::_posteffectsResolveLayouts(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_hdrBuffer,
                                ImageSlice(*_hdrBuffer),
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                0,
                                0,
                                0,
                                0);
}

void ColorFrameBuilder::makeGui()
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGuiWindow window("Color frame");
  if (window.visible())
  {
    _posteffects.makeGui();
    window.end();
  }
}
