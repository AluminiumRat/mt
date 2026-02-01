#include <ImGui.h>

#include <gui/ImGuiPropertyGrid.h>
#include <hld/colorFrameBuilder/Bloom.h>
#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/Device.h>

using namespace mt;

Bloom::Bloom(Device& device) :
  _device(device),
  _treshold(1.0f),
  _intensity(0.1f),
  _sourceImageChanged(false),
  _techniqueConfigurator(new TechniqueConfigurator(device, "Bloom technique")),
  _bloorTechnique(*_techniqueConfigurator),
  _horizontalPass(_bloorTechnique.getOrCreatePass("Horizontal")),
  _verticalPass(_bloorTechnique.getOrCreatePass("Vertical")),
  _sourceImageBinding(_bloorTechnique.getOrCreateResourceBinding(
                                                              "sourceImage")),
  _targetImageBinding(_bloorTechnique.getOrCreateResourceBinding(
                                                              "targetImage")),
  _invSourceSizeUniform(_bloorTechnique.getOrCreateUniform(
                                                      "params.invSourceSize")),
  _targetSizeUniform(_bloorTechnique.getOrCreateUniform("params.targetSize")),
  _tresholdUniform(_bloorTechnique.getOrCreateUniform("params.threshold")),
  _intensityUniform(_bloorTechnique.getOrCreateUniform("params.intensity"))
{
  loadConfigurator( *_techniqueConfigurator, "posteffects/bloom.tch");
  _techniqueConfigurator->rebuildConfiguration();

  _tresholdUniform.setValue(_treshold);
  _intensityUniform.setValue(_intensity);
}

void Bloom::update(CommandProducerCompute& commandProducer)
{
  MT_ASSERT(_sourceImage != nullptr);

  try
  {
    commandProducer.beginDebugLabel("Bloom");

    if(_sourceImageChanged) _updateBindings();
    _bloor(commandProducer);

    commandProducer.endDebugLabel();
  }
  catch(...)
  {
    _bloomImage = nullptr;
    throw;
  }
}

void Bloom::_updateBindings()
{
  _sourceImageBinding.setImage(_sourceImage);
  _invSourceSizeUniform.setValue(1.0f / glm::vec2(_sourceImage->extent()));

  glm::uvec3 bloomImageExtent = _sourceImage->extent();
  bloomImageExtent /= 4;
  bloomImageExtent = glm::max(bloomImageExtent, glm::uvec3(1, 1, 1));

  _targetSizeUniform.setValue(glm::uvec2(bloomImageExtent));

  _bloomImage = new Image(_device,
                          VK_IMAGE_TYPE_2D,
                          VK_IMAGE_USAGE_STORAGE_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                          0,
                          _sourceImage->image().format(),
                          bloomImageExtent,
                          VK_SAMPLE_COUNT_1_BIT,
                          1,
                          1,
                          false,
                          "Bloom");
  Ref<ImageView> targetImageView(new ImageView( *_bloomImage,
                                                ImageSlice(*_bloomImage),
                                                VK_IMAGE_VIEW_TYPE_2D));
  _targetImageBinding.setImage(targetImageView);

  _sourceImageChanged = false;
}

void Bloom::_bloor(CommandProducerCompute& commandProducer)
{
  commandProducer.imageBarrier( *_bloomImage,
                                ImageSlice(*_bloomImage),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL,
                                0,
                                0,
                                0,
                                0);

  //  Горизонтальный проход
  Technique::BindCompute bindHorizontal(_bloorTechnique,
                                        _horizontalPass,
                                        commandProducer);
  MT_ASSERT(bindHorizontal.isValid());
  commandProducer.dispatch(1, _bloomImage->extent().y, 1);
  bindHorizontal.release();

  //  Сбрасываем кэш между проходами
  commandProducer.memoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);

  //  Вертикальный проход
  Technique::BindCompute bindVertical(_bloorTechnique,
                                      _verticalPass,
                                      commandProducer);
  MT_ASSERT(bindVertical.isValid());
  commandProducer.dispatch(_bloomImage->extent().x, 1, 1);
  bindVertical.release();

  commandProducer.imageBarrier( *_bloomImage,
                                ImageSlice(*_bloomImage),
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}

void Bloom::makeGui()
{
  ImGui::Text("Bloom");
  ImGuiPropertyGrid grid("Bloom");

  grid.addRow("Treshold");
  if(ImGui::InputFloat("#treshold", &_treshold))
  {
    _tresholdUniform.setValue(_treshold);
  }

  grid.addRow("Intensity");
  if(ImGui::InputFloat("#intensity", &_intensity))
  {
    _intensityUniform.setValue(_intensity);
  }
}
