#include <ImGui.h>

#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <hld/colorFrameBuilder/Posteffects.h>
#include <hld/FrameBuildContext.h>
#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

Posteffects::Posteffects(Device& device) :
  _device(device),
  _needUpdateBindings(false),
  _avgLum(device),
  _bloom(device),
  _resolveConfigurator(new TechniqueConfigurator(device, "HDRResolve")),
  _resolveTechnique(*_resolveConfigurator),
  _resolvePass(_resolveTechnique.getOrCreatePass("ResolvePass")),
  _hdrBufferBinding(_resolveTechnique.getOrCreateResourceBinding("hdrTexture")),
  _avgLuminanceBinding(
                  _resolveTechnique.getOrCreateResourceBinding("avgLuminance")),
  _bloomTextureBinding(
                  _resolveTechnique.getOrCreateResourceBinding("bloomTexture")),
  _brightnessUniform(_resolveTechnique.getOrCreateUniform("params.brightness")),
  _brightness(1.0f),
  _maxWhiteUniform(_resolveTechnique.getOrCreateUniform("params.maxWhite")),
  _maxWhite(2.0f),
  _bloomEnabledSelection(
                      _resolveTechnique.getOrCreateSelection("BLOOM_ENABLED")),
  _bloomEnabled(true)
{
  loadConfigurator(*_resolveConfigurator, "posteffects/posteffects.tch");
  _resolveConfigurator->rebuildConfiguration();

  _updateProperties();
}

void Posteffects::makeLDR(FrameBuffer& target,
                          CommandProducerGraphic& commandProducer,
                          const FrameBuildContext& frameContext)
{
  MT_ASSERT(_hdrBuffer != nullptr);

  //  Готвим данные, необходимые для резолва HDR
  _avgLum.update(commandProducer);
  if(_bloomEnabled) _bloom.update(commandProducer);

  _updateBindings();

  //  Резолв hdr и постэффекты в одной отрисовке
  CommandProducerGraphic::RenderPass renderPass(commandProducer, target);
    Technique::BindGraphic bind(_resolveTechnique,
                                _resolvePass,
                                commandProducer);
    MT_ASSERT(bind.isValid())
    commandProducer.draw(4);
  renderPass.endPass();
}

void Posteffects::_updateBindings()
{
  MT_ASSERT(_hdrBuffer != nullptr);

  if(!_needUpdateBindings) return;

  _hdrBufferBinding.setImage(_hdrBuffer);
  _avgLuminanceBinding.setBuffer(&_avgLum.resultBuffer());

  if(_bloomEnabled)
  {
    MT_ASSERT(_bloom.bloomImage() != nullptr);
    Ref<ImageView> bloomTextureView(new ImageView(
                                              *_bloom.bloomImage(),
                                              ImageSlice(*_bloom.bloomImage()),
                                              VK_IMAGE_VIEW_TYPE_2D));
    _bloomTextureBinding.setImage(bloomTextureView);
  }
  else _bloomTextureBinding.setImage(nullptr);

  _needUpdateBindings = false;
}

void Posteffects::_updateProperties()
{
  _brightnessUniform.setValue(_brightness);
  _maxWhiteUniform.setValue(_maxWhite);
  _bloomEnabledSelection.setValue(_bloomEnabled ? "1" : "0");
}

void Posteffects::makeGui()
{
  ImGuiTreeNode node("Posteffects");
  if(node.open())
  {
    {
      ImGui::Text("Color grading");
      ImGuiPropertyGrid grid("ColorGrading");

      grid.addRow("brightness");
      if(ImGui::InputFloat("#Brightness", &_brightness)) _updateProperties();

      grid.addRow("MaxWhite");
      if(ImGui::InputFloat("#maxwhite", &_maxWhite)) _updateProperties();
    }

    if(ImGui::Checkbox("Bloom", &_bloomEnabled))
    {
      _needUpdateBindings = true;
      _updateProperties();
    }
    if(_bloomEnabled) _bloom.makeGui();
  }
}
