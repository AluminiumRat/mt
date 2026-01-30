#include <ImGui.h>

#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/Posteffects.h>
#include <hld/FrameBuildContext.h>
#include <technique/TechniqueLoader.h>
#include <vkr/image/ImageView.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

Posteffects::Posteffects(Device& device) :
  _device(device),
  _hdrBufferChanged(false),
  _luminancePyramid(_device),
  _resolveConfigurator(new TechniqueConfigurator(device, "HDRResolve")),
  _resolveTechnique(*_resolveConfigurator),
  _resolvePass(_resolveTechnique.getOrCreatePass("ResolvePass")),
  _hdrBufferBinding(_resolveTechnique.getOrCreateResourceBinding("hdrTexture")),
  _luminancePyramidBinding(
            _resolveTechnique.getOrCreateResourceBinding("luminancePyramid")),
  _avgColorBinding(_resolveTechnique.getOrCreateResourceBinding("avgColor")),
  _brightnessUniform(_resolveTechnique.getOrCreateUniform("params.brightness")),
  _brightness(1.0f),
  _maxWhiteUniform(_resolveTechnique.getOrCreateUniform("params.maxWhite")),
  _maxWhite(2.0f)
{
  loadConfigurator(*_resolveConfigurator, "posteffects/posteffects.tch");
  _resolveConfigurator->rebuildConfiguration();

  _brightnessUniform.setValue(_brightness);
  _maxWhiteUniform.setValue(_maxWhite);
}

void Posteffects::prepare(CommandProducerGraphic& commandProducer,
                          const FrameBuildContext& frameContext)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  _luminancePyramid.update(commandProducer, *_hdrBuffer);
}

void Posteffects::makeLDR(CommandProducerGraphic& commandProducer,
                          const FrameBuildContext& frameContext)
{
  _updateBindings();

  Technique::BindGraphic bind(_resolveTechnique, _resolvePass, commandProducer);
  MT_ASSERT(bind.isValid())

  commandProducer.draw(4);
}

void Posteffects::_updateBindings()
{
  MT_ASSERT(_hdrBuffer != nullptr);
  MT_ASSERT(_luminancePyramid.pyramidImage() != nullptr);

  if(!_hdrBufferChanged) return;

  Ref<ImageView> hdrView(new ImageView( *_hdrBuffer,
                                        ImageSlice(*_hdrBuffer),
                                        VK_IMAGE_VIEW_TYPE_2D));
  _hdrBufferBinding.setImage(hdrView);

  Image& pyramid = *_luminancePyramid.pyramidImage();
  Ref<ImageView> luminancePyramidView( new ImageView(pyramid,
                                                      ImageSlice(pyramid),
                                                      VK_IMAGE_VIEW_TYPE_2D));
  _luminancePyramidBinding.setImage(luminancePyramidView);

  Ref<ImageView> avgColorView(new ImageView(
                                          pyramid,
                                          ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                                      pyramid.mipmapCount() - 1,
                                                      1,
                                                      0,
                                                      1),
                                          VK_IMAGE_VIEW_TYPE_2D));
  _avgColorBinding.setImage(avgColorView);

  _hdrBufferChanged = false;
}

void Posteffects::makeGui()
{
  ImGuiTreeNode node("Posteffects");
  if(node.open())
  {
    ImGuiPropertyGrid grid("Posteffects");

    grid.addRow("brightness");
    if(ImGui::InputFloat("#Brightness", &_brightness))
    {
      _brightnessUniform.setValue(_brightness);
    }

    grid.addRow("MaxWhite");
    if(ImGui::InputFloat("#maxwhite", &_maxWhite))
    {
      _maxWhiteUniform.setValue(_maxWhite);
    }
  }
}
