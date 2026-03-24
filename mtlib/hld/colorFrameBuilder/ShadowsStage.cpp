#include <ImGui.h>

#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/ShadowsStage.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/FrameBuildContext.h>
#include <resourceManagement/TextureManager.h>
#include <technique/TechniqueLoader.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ShadowsStage::ShadowsStage(Device& device, TextureManager& textureManager) :
  _device(device),
  _rayQueryTechniqueConfigurator(new TechniqueConfigurator( device,
                                                            "RayQueryShadows")),
  _rayQueryTechnique(*_rayQueryTechniqueConfigurator),
  _rayQueryPass(_rayQueryTechnique.getOrCreatePass("RayTracePass")),
  _spatialFilterPass(_rayQueryTechnique.getOrCreatePass("SpatialFilterPass")),
  _tlasBinding(_rayQueryTechnique.getOrCreateResourceBinding("tlas")),
  _noiseTextureBinding(
                _rayQueryTechnique.getOrCreateResourceBinding("noiseTexture")),
  _samplerTextureBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("samplerTexture")),
  _rawShadowMaskBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("rawShadowMask")),
  _prevShadowMaskBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("prevShadowMask")),
  _finalShadowMaskBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("finalShadowMask")),
  _rayForwardShiftUniform(
              _rayQueryTechnique.getOrCreateUniform("params.rayForwardShift")),
  _rayNormalShiftUniform(
              _rayQueryTechnique.getOrCreateUniform("params.rayNormalShift")),
  _rayForwardShift(0.5f),
  _rayNormalShift(2.0f),
  _targetBuffer(0),
  _gridSize(1)
{
  ConstRef<TechniqueResource> noiseTexture =
                        textureManager.loadImmediately( "util/noiseR8x32.dds",
                                                        *device.graphicQueue(),
                                                        false);
  if(noiseTexture->image() == nullptr) throw std::runtime_error("ShadowsStage: unable to load 'util/noiseR8x32.dds'");
  _noiseTextureBinding.setResource(noiseTexture);

  ConstRef<TechniqueResource> samplerTexture =
                          textureManager.loadImmediately(
                                                    "util/diskSampler1024.dds",
                                                    *device.graphicQueue(),
                                                    false);
  if(samplerTexture->image() == nullptr) throw std::runtime_error("ShadowsStage: unable to load 'util/diskSampler1024.dds'");
  _samplerTextureBinding.setResource(samplerTexture);

  if(device.features().rayQuery.rayQuery == VK_TRUE)
  {
    loadConfigurator( *_rayQueryTechniqueConfigurator,
                      "shadows/rayQueryShadows.tch");
    _rayQueryTechniqueConfigurator->rebuildConfiguration();
  }

  _rayForwardShiftUniform.setValue(_rayForwardShift);
  _rayNormalShiftUniform.setValue(_rayNormalShift);
}

void ShadowsStage::draw(CommandProducerGraphic& commandProducer,
                        const FrameBuildContext& frameContext)
{
  if(_rayShadowBuffers[0] == nullptr) _createBuffers(commandProducer);

  const TLAS* tlas = frameContext.drawScene->tlas();
  _tlasBinding.setTLAS(tlas);

  _rawShadowMaskBinding.setImage(_rayShadowBuffers[_targetBuffer]);
  _targetBuffer = (_targetBuffer + 1) % 2;
  _prevShadowMaskBinding.setImage(_rayShadowBuffers[_targetBuffer]);

  if(_rayQueryTechnique.isReady())
  {
    {
      Technique::BindCompute bind(_rayQueryTechnique,
                                  _rayQueryPass,
                                  commandProducer);
      MT_ASSERT(bind.isValid())
      commandProducer.dispatch(_gridSize);
    }

    commandProducer.memoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  VK_ACCESS_SHADER_WRITE_BIT,
                                  VK_ACCESS_SHADER_READ_BIT);
    {
      Technique::BindCompute bind(_rayQueryTechnique,
                                  _spatialFilterPass,
                                  commandProducer);
      MT_ASSERT(bind.isValid())
      commandProducer.dispatch(_gridSize);
    }
  }
}

void ShadowsStage::_createBuffers(CommandProducerGraphic& commandProducer)
{
  MT_ASSERT(_shadowBuffer != nullptr);

  _gridSize = (glm::uvec2(_shadowBuffer->extent()) + glm::uvec2(7)) / 8u;

  //  Буферы для трэйса теней
  for(int i = 0; i < 2; i++)
  {
    ConstRef<Image> rayShadowsBufferImage(new Image(
                                                _device,
                                                VK_IMAGE_TYPE_2D,
                                                VK_IMAGE_USAGE_STORAGE_BIT |
                                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                                0,
                                                VK_FORMAT_R8_UNORM,
                                                _shadowBuffer->extent(),
                                                VK_SAMPLE_COUNT_1_BIT,
                                                1,
                                                1,
                                                true,
                                                "ShadowsStage::TracingBuffer"));
    //commandProducer.initLayout(*rayShadowsBufferImage, VK_IMAGE_LAYOUT_GENERAL);
    _rayShadowBuffers[i] = new ImageView( *rayShadowsBufferImage,
                                          ImageSlice(*rayShadowsBufferImage),
                                          VK_IMAGE_VIEW_TYPE_2D);
  }

  _finalShadowMaskBinding.setImage(_shadowBuffer);
}

void ShadowsStage::makeGui()
{
  ImGuiPropertyGrid grid("Shadows");

  grid.addRow("rayForwardShift");
  float forwardShiftValue = _rayForwardShift;
  if(ImGui::InputFloat("#rayForwardShift", &forwardShiftValue))
  {
    setRayForwardShift(forwardShiftValue);
  }

  grid.addRow("rayNormalShift");
  float normalShiftValue = _rayNormalShift;
  if(ImGui::InputFloat("#rayNormalShift", &normalShiftValue))
  {
    setRayNormalShift(normalShiftValue);
  }
}
