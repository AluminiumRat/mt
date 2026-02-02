#include <hld/colorFrameBuilder/AvgLum.h>
#include <technique/TechniqueLoader.h>
#include <util/floorPow.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/Device.h>

using namespace mt;

AvgLum::AvgLum(Device& device) :
  _device(device),
  _sourceImageChanged(false),
  _resultBuffer(new DataBuffer( device,
                                sizeof(float),
                                DataBuffer::STORAGE_BUFFER,
                                "AvgLuminanceBuffer")),
  _techniqueConfigurator(new TechniqueConfigurator(device, "AvgLum technique")),
  _technique(*_techniqueConfigurator),
  _horizontalPass(_technique.getOrCreatePass("Horizontal")),
  _verticalPass(_technique.getOrCreatePass("Vertical")),
  _sourceImageBinding(_technique.getOrCreateResourceBinding("sourceImage")),
  _resultBufferBinding(_technique.getOrCreateResourceBinding("avgLumBuffer")),
  _intermediateImageBinding(_technique.getOrCreateResourceBinding(
                                                          "intermediateImage")),
  _invSourceSizeUniform(_technique.getOrCreateUniform("params.invSourceSize")),
  _areaSizeUniform(_technique.getOrCreateUniform("params.areaSize"))
{
  loadConfigurator( *_techniqueConfigurator, "posteffects/avgLum.tch");
  _techniqueConfigurator->rebuildConfiguration();
  _resultBufferBinding.setBuffer(_resultBuffer);
}

void AvgLum::update(CommandProducerCompute& commandProducer)
{
  MT_ASSERT(_sourceImage != nullptr);

  commandProducer.beginDebugLabel("AvgLum");

  if(_sourceImageChanged) _updateBindings(commandProducer);
  _average(commandProducer);

  commandProducer.endDebugLabel();
}

void AvgLum::_updateBindings(CommandProducerCompute& commandProducer)
{
  _sourceImageBinding.setImage(_sourceImage);
  _invSourceSizeUniform.setValue(1.0f / glm::vec2(_sourceImage->extent()));

  _workAreaSize = _sourceImage->extent();
  _workAreaSize = floorPow(_workAreaSize);
  _workAreaSize /= 4;
  _workAreaSize = glm::max(_workAreaSize, glm::uvec2(1, 1));

  _areaSizeUniform.setValue(glm::uvec2(_workAreaSize));

  _intermediateImage = new Image( _device,
                                  VK_IMAGE_TYPE_2D,
                                  VK_IMAGE_USAGE_STORAGE_BIT,
                                  0,
                                  VK_FORMAT_R32_SFLOAT,
                                  glm::uvec3(1, _workAreaSize.y, 1),
                                  VK_SAMPLE_COUNT_1_BIT,
                                  1,
                                  1,
                                  false,
                                  "AVGLumIntermediateImage");
  Ref<ImageView> intermediateImageView(new ImageView(
                                                *_intermediateImage,
                                                ImageSlice(*_intermediateImage),
                                                VK_IMAGE_VIEW_TYPE_2D));
  _intermediateImageBinding.setImage(intermediateImageView);

  commandProducer.imageBarrier( *_intermediateImage,
                                ImageSlice(*_intermediateImage),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL,
                                0,
                                0,
                                0,
                                0);

  _sourceImageChanged = false;
}

void AvgLum::_average(CommandProducerCompute& commandProducer)
{
  //  Горизонтальный проход
  Technique::BindCompute bindHorizontal(_technique,
                                        _horizontalPass,
                                        commandProducer);
  MT_ASSERT(bindHorizontal.isValid());
  commandProducer.dispatch(1, _workAreaSize.y, 1);
  bindHorizontal.release();

  //  Сбрасываем кэш между проходами
  commandProducer.memoryBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);

  //  Вертикальный проход
  Technique::BindCompute bindVertical(_technique,
                                      _verticalPass,
                                      commandProducer);
  MT_ASSERT(bindVertical.isValid());
  commandProducer.dispatch(1, 1, 1);
  bindVertical.release();
}
