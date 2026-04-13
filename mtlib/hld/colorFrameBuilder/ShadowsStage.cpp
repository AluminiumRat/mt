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
  _enabled(true),
  _rayQueryTechniqueConfigurator(new TechniqueConfigurator( device,
                                                            "RayQueryShadows")),
  _rayQueryTechnique(*_rayQueryTechniqueConfigurator),
  _rayQueryPass(_rayQueryTechnique.getOrCreatePass("RayTracePass")),
  _variationHorizontalPass(
                _rayQueryTechnique.getOrCreatePass("VariationHorizontalPass")),
  _variationVerticalPass(
                _rayQueryTechnique.getOrCreatePass("VariationVerticalPass")),
  _horizontalFilterPass(
                    _rayQueryTechnique.getOrCreatePass("FilterHorizontalPass")),
  _verticalFilterPass(_rayQueryTechnique.getOrCreatePass("FilterVerticalPass")),
  _tlasBinding(_rayQueryTechnique.getOrCreateResourceBinding("tlas")),
  _noiseTextureBinding(
                _rayQueryTechnique.getOrCreateResourceBinding("noiseTexture")),
  _samplerTextureBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("samplerTexture")),
  _traceResultsBufferBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("traceResults")),
  _samplesCountTextureBinding(
          _rayQueryTechnique.getOrCreateResourceBinding("samplesCountBuffer")),
  _prevSamplesCountTextureBinding(
      _rayQueryTechnique.getOrCreateResourceBinding("prevSamplesCountBuffer")),
  _prevTraceResultsBufferBinding(
            _rayQueryTechnique.getOrCreateResourceBinding("traceResultsPrev")),
  _variationBufferBinding(
              _rayQueryTechnique.getOrCreateResourceBinding("variationBuffer")),
  _rayForwardShiftUniform(
              _rayQueryTechnique.getOrCreateUniform("params.rayForwardShift")),
  _rayNormalShiftUniform(
              _rayQueryTechnique.getOrCreateUniform("params.rayNormalShift")),
  _rayForwardShift(0.5f),
  _rayNormalShift(2.0f),
  _gridSize(1)
{
  if(device.features().rayQuery.rayQuery == VK_TRUE)
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

    loadConfigurator( *_rayQueryTechniqueConfigurator,
                      "shadows/rayQueryShadows.tch");
  }

  _rayForwardShiftUniform.setValue(_rayForwardShift);
  _rayNormalShiftUniform.setValue(_rayNormalShift);
}

void ShadowsStage::draw(CommandProducerGraphic& commandProducer,
                        const FrameBuildContext& frameContext)
{
  const TLAS* tlas = frameContext.drawScene->tlas();

  if( !_enabled ||
      _device.features().rayQuery.rayQuery != VK_TRUE ||
      tlas == nullptr)
  {
    //  Тени отключены или не могут быть посчитаны. Просто чистим маску
    commandProducer.clearColorImage(_shadowBuffer->image(),
                                    VkClearColorValue{.float32 = {1, 1, 1, 1}});
    return;
  }

  if(_traceResultBuffers[0] == nullptr)
  {
    //  Был вызван setBuffers, необходимо пересоздать буферы под новый
    //  размер и перенастроить технику
    _createBuffers(commandProducer);
    _rebuildTechnique();
  }

  _swapBuffers(commandProducer);
  _tlasBinding.setTLAS(tlas);

  MT_ASSERT(_rayQueryTechnique.isReady());

  //  Трэйс теней
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

  //  Построение variationMap. Горизонтальное размытие
  {
    Technique::BindCompute bind(_rayQueryTechnique,
                                _variationHorizontalPass,
                                commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(1, _shadowBuffer->extent().y);
  }
  //  Фильтрация маски теней. Горизонтальное размытие
  {
    Technique::BindCompute bind(_rayQueryTechnique,
                                _horizontalFilterPass,
                                commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(_gridSize);
  }

  commandProducer.memoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);

  //  Построение variationMap. Вертикальное размытие
  {
    Technique::BindCompute bind(_rayQueryTechnique,
                                _variationVerticalPass,
                                commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(_shadowBuffer->extent().x, 1);
  }
  //  Фильтрация маски теней. Вертикальное размытие
  {
    Technique::BindCompute bind(_rayQueryTechnique,
                                _verticalFilterPass,
                                commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(_gridSize);
  }
}

void ShadowsStage::_resetBuffers() noexcept
{
  _samplesCountBuffers[0].reset();
  _samplesCountBuffers[1].reset();
  _traceResultBuffers[0].reset();
  _traceResultBuffers[1].reset();
  _variationBuffer.reset();
}

void ShadowsStage::_createBuffers(CommandProducerGraphic& commandProducer)
{
  MT_ASSERT(_shadowBuffer != nullptr);
  glm::uvec2 buffersExtent = _shadowBuffer->extent();

  for(int i = 0; i < 2; i++)
  {
    ConstRef<Image> traceResultsBufferImage(new Image(
                                                _device,
                                                VK_IMAGE_USAGE_STORAGE_BIT |
                                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_FORMAT_R8G8B8A8_UNORM,
                                                buffersExtent,
                                                "ShadowsStage::TracingBuffer"));
    commandProducer.initLayout( *traceResultsBufferImage,
                                i == 0 ?
                                      VK_IMAGE_LAYOUT_GENERAL :
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    _traceResultBuffers[i] = new ImageView(*traceResultsBufferImage);

    ConstRef<Image> samplesCountBufferImage(new Image(
                                                _device,
                                                VK_IMAGE_USAGE_STORAGE_BIT |
                                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_FORMAT_R8G8B8A8_SINT,
                                                buffersExtent,
                                                "ShadowsStage::SamplesCountBuffer"));
    commandProducer.initLayout( *samplesCountBufferImage,
                                i == 0 ?
                                      VK_IMAGE_LAYOUT_GENERAL :
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    _samplesCountBuffers[i] = new ImageView(*samplesCountBufferImage);
  }

  ConstRef<Image> variationBufferImage(new Image(
                                              _device,
                                              VK_IMAGE_USAGE_STORAGE_BIT,
                                              VK_FORMAT_R8_UNORM,
                                              buffersExtent,
                                              "ShadowsStage::VariationBuffer"));
  commandProducer.initLayout(*variationBufferImage, VK_IMAGE_LAYOUT_GENERAL);
  _variationBuffer = new ImageView(*variationBufferImage);
  _variationBufferBinding.setImage(_variationBuffer);
}

void ShadowsStage::_rebuildTechnique()
{
  //  Технику нужно пересобирать каждый раз при изменении размеров размываемой
  //  картинки. Это необходимо из-за настройки размеров групп для шейдеров.
  //  Ищем проходы, для которых надо настроить константы специализации,
  //  настраиваем их, и пересобираем технику.

  if(_device.features().rayQuery.rayQuery != VK_TRUE) return;

  constexpr uint32_t maxGroupSize = 256;

  // Pixel per invocation, количество пикселей на 1 поток на GPU
  uint32_t ppiHorizontal =
                  (_shadowBuffer->extent().x + maxGroupSize - 1) / maxGroupSize;
  uint32_t horizontalGroupSize =
                (_shadowBuffer->extent().x + ppiHorizontal - 1) / ppiHorizontal;
  uint32_t ppiVertical =
                  (_shadowBuffer->extent().y + maxGroupSize - 1) / maxGroupSize;
  uint32_t verticalGroupSize =
                (_shadowBuffer->extent().y + ppiHorizontal - 1) / ppiHorizontal;

  const TechniqueConfigurator::Passes& passes =
                                      _rayQueryTechniqueConfigurator->passes();
  for(const std::unique_ptr<PassConfigurator>& pass : passes)
  {
    if(pass->name() == _variationHorizontalPass.name())
    {
      MT_ASSERT(!pass->shaders().empty());
      PassConfigurator::ShaderInfo shader = pass->shaders()[0];
      shader.constants.clear();
      shader.constants.addConstant("GROUP_SIZE", horizontalGroupSize);
      shader.constants.addConstant("PPI", ppiHorizontal);
      pass->setShaders(std::span(&shader, 1));
    }
    if(pass->name() == _variationVerticalPass.name())
    {
      MT_ASSERT(!pass->shaders().empty());
      PassConfigurator::ShaderInfo shader = pass->shaders()[0];
      shader.constants.clear();
      shader.constants.addConstant("GROUP_SIZE", verticalGroupSize);
      shader.constants.addConstant("PPI", ppiVertical);
      pass->setShaders(std::span(&shader, 1));
    }
  }

  _rayQueryTechniqueConfigurator->rebuildConfiguration();
}

void ShadowsStage::_swapBuffers(CommandProducerGraphic& commandProducer)
{
  std::swap(_traceResultBuffers[0], _traceResultBuffers[1]);
  commandProducer.imageBarrier( _traceResultBuffers[0]->image(),
                                ImageSlice(_traceResultBuffers[0]->image()),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL,
                                0,
                                0,
                                0,
                                0);
  commandProducer.imageBarrier( _traceResultBuffers[1]->image(),
                                ImageSlice(_traceResultBuffers[1]->image()),
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                0,
                                0,
                                0,
                                0);
  _traceResultsBufferBinding.setImage(_traceResultBuffers[0]);
  _prevTraceResultsBufferBinding.setImage(_traceResultBuffers[1]);

  std::swap(_samplesCountBuffers[0], _samplesCountBuffers[1]);
  commandProducer.imageBarrier( _samplesCountBuffers[0]->image(),
                                ImageSlice(_samplesCountBuffers[0]->image()),
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL,
                                0,
                                0,
                                0,
                                0);
  commandProducer.imageBarrier( _samplesCountBuffers[1]->image(),
                                ImageSlice(_samplesCountBuffers[1]->image()),
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                0,
                                0,
                                0,
                                0);
  _samplesCountTextureBinding.setImage(_samplesCountBuffers[0]);
  _prevSamplesCountTextureBinding.setImage(_samplesCountBuffers[1]);
}

void ShadowsStage::makeGui()
{
  if(_device.features().rayQuery.rayQuery != VK_TRUE)
  {
    ImGui::Text("WARNING! Ray queries aren't available.");
    ImGui::Text("Shadows are disabled.");
  }
  bool enabledValue = enabled();
  if(ImGui::Checkbox("enabled", &enabledValue)) setEnabled(enabledValue);

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
