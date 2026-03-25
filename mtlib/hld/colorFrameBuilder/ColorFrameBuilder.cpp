#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/FrameBuildContext.h>
#include <hld/HLDLib.h>
#include <technique/DescriptorSetType.h>
#include <util/Camera.h>
#include <util/floorPow.h>
#include <vkr/image/ImageView.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ColorFrameBuilder::ColorFrameBuilder( Device& device,
                                      TextureManager& textureManager,
                                      TechniqueManager& techniqueManager) :
  _device(device),
  _frameTypeIndex(HLDLib::instance().getFrameTypeIndex(frameTypeName)),
  _commonSet(device, textureManager),
  _opaquePrepassStage(device),
  _reprojectionBufferUpdater(device),
  _shadowsStage(device, textureManager),
  _opaqueColorStage(device),
  _backgroundRender(device, techniqueManager),
  _posteffects(device)
{
}

void ColorFrameBuilder::draw( FrameBuffer& target,
                              const DrawScene& scene,
                              const Camera& viewCamera,
                              const EnvironmentScene& environment)
{
  //  Определяем регион в который мы будем рисовать новое изображение
  Region targetRegion = _drawRegion.valid() ? _drawRegion : target.extent();
  if(!targetRegion.valid()) return;

  _updateBuffers(targetRegion.size());

  FrameBuildContext frameContext{};
  frameContext.frameType = _frameTypeIndex;
  frameContext.viewCamera = &viewCamera;
  frameContext.drawScene = &scene;
  frameContext.frameExtent = targetRegion.size();

  _drawPlan.clear();
  scene.fillDrawPlan(_drawPlan, frameContext);

  {
    //  Подготовительные работы
    std::unique_ptr<CommandProducerGraphic> prepareProducer =
                                        _device.graphicQueue()->startCommands();
    _initBuffersLayout(*prepareProducer);
    _commonSet.update(*prepareProducer,
                      frameContext,
                      environment,
                      *_halfLinearDepthBufferView,
                      *_halfNormalBufferView,
                      *_reprojectionBufferView,
                      *_shadowBufferView);
    _device.graphicQueue()->submitCommands(std::move(prepareProducer));
  }

  {
    //  Предварительный проход
    std::unique_ptr<CommandProducerGraphic> preopaqueProducer =
                                  _device.graphicQueue()->startCommands(
                                                OpaquePrepassStage::stageName);
    {
      ColorFrameCommonSet::Bind bindCommonSet(_commonSet, *preopaqueProducer);
      _opaquePrepassStage.draw( *preopaqueProducer,
                                _drawPlan,
                                frameContext);
      _reprojectionStageLayout(*preopaqueProducer);
      _reprojectionBufferUpdater.updateReprojection(*preopaqueProducer);
    }
    _device.graphicQueue()->submitCommands(std::move(preopaqueProducer));
  }

  {
    //  Отрисовка теней
    std::unique_ptr<CommandProducerGraphic> shadowsProducer =
                              _device.graphicQueue()->startCommands("Shadows");
    _shadowsLayout(*shadowsProducer);
    {
      ColorFrameCommonSet::Bind bindCommonSet(_commonSet, *shadowsProducer);
      _shadowsStage.draw(*shadowsProducer, frameContext);
    }
    _device.graphicQueue()->submitCommands(std::move(shadowsProducer));
  }

  {
    //  Opaque проход
    std::unique_ptr<CommandProducerGraphic> opaqueProducer =
                                  _device.graphicQueue()->startCommands(
                                                  OpaqueColorStage::stageName);
    _opaquePassLayout(*opaqueProducer);
    {
      ColorFrameCommonSet::Bind bindCommonSet(_commonSet, *opaqueProducer);
      _opaqueColorStage.draw( *opaqueProducer,
                              _drawPlan,
                              frameContext);
      _backgroundRender.draw(*opaqueProducer);
    }
    _device.graphicQueue()->submitCommands(std::move(opaqueProducer));
  }

  {
    //  Конечная сборка кадра
    std::unique_ptr<CommandProducerGraphic> ldrProducer =
                              _device.graphicQueue()->startCommands("LDRStage");

    _posteffectsLayouts(*ldrProducer);
    _posteffects.makeLDR(target, targetRegion, *ldrProducer);

    _device.graphicQueue()->submitCommands(std::move(ldrProducer));
  }
}

void ColorFrameBuilder::_updateBuffers(glm::uvec2 targetExtent)
{
  //  Проверка, а надо ли вообще пересоздавать буферы
  if (_hdrBuffer != nullptr &&
      glm::uvec2(_hdrBuffer->extent()) == targetExtent)
  {
    return;
  }

  glm::uvec2 halfSize = glm::max(targetExtent / 2u, glm::uvec2(1));

  _hdrBuffer = new Image( _device,
                          VK_IMAGE_TYPE_2D,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                          0,
                          hdrFormat,
                          glm::uvec3(targetExtent, 1),
                          VK_SAMPLE_COUNT_1_BIT,
                          1,
                          1,
                          false,
                          "HDRBuffer");
  _hdrBufferView = new ImageView( *_hdrBuffer,
                                  ImageSlice(*_hdrBuffer),
                                  VK_IMAGE_VIEW_TYPE_2D);

  _depthBuffer = new Image( _device,
                            VK_IMAGE_TYPE_2D,
                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            0,
                            depthBufferFormat,
                            glm::uvec3(targetExtent, 1),
                            VK_SAMPLE_COUNT_1_BIT,
                            1,
                            1,
                            false,
                            "DepthBuffer");
  _depthBufferView = new ImageView( *_depthBuffer,
                                    ImageSlice(*_depthBuffer),
                                    VK_IMAGE_VIEW_TYPE_2D);

  _halfDepthBuffer = new Image( _device,
                                VK_IMAGE_TYPE_2D,
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                0,
                                depthBufferFormat,
                                glm::uvec3(halfSize, 1),
                                VK_SAMPLE_COUNT_1_BIT,
                                1,
                                1,
                                false,
                                "DepthHalfBuffer");
  _halfDepthBufferView = new ImageView( *_halfDepthBuffer,
                                        ImageSlice(*_halfDepthBuffer),
                                        VK_IMAGE_VIEW_TYPE_2D);
  _halfLinearDepthBuffer = new Image( _device,
                                      VK_IMAGE_TYPE_2D,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                                      0,
                                      linearDepthFormat,
                                      glm::uvec3(halfSize, 1),
                                      VK_SAMPLE_COUNT_1_BIT,
                                      1,
                                      1,
                                      false,
                                      "LinearDepthHalfBuffer");
  _halfLinearDepthBufferView = new ImageView(
                                            *_halfLinearDepthBuffer,
                                            ImageSlice(*_halfLinearDepthBuffer),
                                            VK_IMAGE_VIEW_TYPE_2D);
  
  _halfNormalBuffer = new Image(_device,
                                VK_IMAGE_TYPE_2D,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                0,
                                halfNormalFormat,
                                glm::uvec3(halfSize, 1),
                                VK_SAMPLE_COUNT_1_BIT,
                                1,
                                1,
                                false,
                                "NormalHalfBuffer");
  _halfNormalBufferView = new ImageView(*_halfNormalBuffer,
                                        ImageSlice(*_halfNormalBuffer),
                                        VK_IMAGE_VIEW_TYPE_2D);

  _reprojectionBuffer = new Image(_device,
                                  VK_IMAGE_TYPE_2D,
                                  VK_IMAGE_USAGE_STORAGE_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT,
                                  0,
                                  reprojectionBufferFormat,
                                  glm::uvec3(halfSize, 1),
                                  VK_SAMPLE_COUNT_1_BIT,
                                  1,
                                  1,
                                  false,
                                  "ReprojectionBuffer");
  _reprojectionBufferView = new ImageView(*_reprojectionBuffer,
                                          ImageSlice(*_reprojectionBuffer),
                                          VK_IMAGE_VIEW_TYPE_2D);

  _shadowBuffer = new Image(_device,
                            VK_IMAGE_TYPE_2D,
                            VK_IMAGE_USAGE_STORAGE_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT,
                            0,
                            shadowFormat,
                            glm::uvec3(halfSize, 1),
                            VK_SAMPLE_COUNT_1_BIT,
                            1,
                            1,
                            false,
                            "ShadowBuffer");
  _shadowBufferView = new ImageView(*_shadowBuffer,
                                    ImageSlice(*_shadowBuffer),
                                    VK_IMAGE_VIEW_TYPE_2D);

  _opaquePrepassStage.setBuffers( *_halfDepthBufferView,
                                  *_halfLinearDepthBufferView,
                                  *_halfNormalBufferView);
  _reprojectionBufferUpdater.setBuffers(*_reprojectionBufferView);
  _shadowsStage.setBuffers(*_shadowBufferView);
  _opaqueColorStage.setBuffers(*_hdrBufferView, *_depthBufferView);
  _backgroundRender.setBuffers(*_hdrBufferView, *_depthBufferView);
  _posteffects.setHdrBuffer(*_hdrBufferView);
}

void ColorFrameBuilder::_initBuffersLayout(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.initLayout( *_hdrBuffer,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  commandProducer.initLayout( *_depthBuffer,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  commandProducer.initLayout( *_halfDepthBuffer,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  commandProducer.initLayout( *_halfLinearDepthBuffer,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  commandProducer.initLayout( *_halfNormalBuffer,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  commandProducer.initLayout( *_reprojectionBuffer, VK_IMAGE_LAYOUT_GENERAL);

  commandProducer.initLayout(*_shadowBuffer, VK_IMAGE_LAYOUT_GENERAL);
}

void ColorFrameBuilder::_reprojectionStageLayout(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_halfLinearDepthBuffer,
                                ImageSlice(*_halfLinearDepthBuffer),
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}

void ColorFrameBuilder::_shadowsLayout(CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_reprojectionBuffer,
                                ImageSlice(*_reprojectionBuffer),
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);

  commandProducer.imageBarrier( *_halfNormalBuffer,
                                ImageSlice(*_halfNormalBuffer),
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}

void ColorFrameBuilder::_opaquePassLayout(
                                      CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_shadowBuffer,
                                ImageSlice(*_shadowBuffer),
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}

void ColorFrameBuilder::_posteffectsLayouts(
                                        CommandProducerGraphic& commandProducer)
{
  commandProducer.imageBarrier( *_hdrBuffer,
                                ImageSlice(*_hdrBuffer),
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}

void ColorFrameBuilder::makeGui()
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGuiWindow window("Color frame");
  if (window.visible())
  {
    {
      ImGuiTreeNode shadowsNode("Shadows");
      if(shadowsNode.open()) _shadowsStage.makeGui();
    }

    {
      ImGuiTreeNode posteffectsNode("Posteffects");
      if(posteffectsNode.open()) _posteffects.makeGui();
    }

    window.end();
  }
}
