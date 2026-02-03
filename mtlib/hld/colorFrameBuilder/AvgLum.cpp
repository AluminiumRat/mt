#include <hld/colorFrameBuilder/AvgLum.h>
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
  _resultBufferBinding.setBuffer(_resultBuffer);

  Ref<Sampler> sampler(new Sampler(_device));
  _technique.getOrCreateResourceBinding("linearSampler").setSampler(sampler);
}

void AvgLum::update(CommandProducerCompute& commandProducer)
{
  MT_ASSERT(_sourceImage != nullptr);

  commandProducer.beginDebugLabel("AvgLum");

  if(_sourceImageChanged)
  {
    _workAreaSize = _sourceImage->extent();
    _workAreaSize = floorPow(_workAreaSize);
    _workAreaSize /= 4;
    _workAreaSize = glm::max(_workAreaSize, glm::uvec2(1, 1));

    _updateTechnique();
    _updateBindings(commandProducer);
  }

  _average(commandProducer);

  commandProducer.endDebugLabel();
}

void AvgLum::_updateTechnique()
{
  //  Технику нужно пересобирать каждый раз при изменении размеров размываемой
  //  картинки. Это необходимо из-за настройки размеров групп для шейдеров.
  //  Настройка производится через константы специализации.
  _techniqueConfigurator->clear();

  TechniqueConfigurator::Passes passes;
  {
    //  Горизонтальный проход
    std::unique_ptr<PassConfigurator> horizontalPass(
                                          new PassConfigurator("Horizontal"));
    horizontalPass->setPipelineType(AbstractPipeline::COMPUTE_PIPELINE);
    PassConfigurator::ShaderInfo shaders[1] =
                                {{.stage = VK_SHADER_STAGE_COMPUTE_BIT,
                                  .file = "posteffects/avgLumHorizontal.comp"}};
    shaders[0].constants.addConstant("GROUP_SIZE", uint32_t(_workAreaSize.x));
    horizontalPass->setShaders(shaders);
    passes.push_back(std::move(horizontalPass));
  }
  {
    //  Вертикальный проход
    std::unique_ptr<PassConfigurator> verticalPass(
                                          new PassConfigurator("Vertical"));
    verticalPass->setPipelineType(AbstractPipeline::COMPUTE_PIPELINE);
    PassConfigurator::ShaderInfo shaders[1] =
                                  {{.stage = VK_SHADER_STAGE_COMPUTE_BIT,
                                    .file = "posteffects/avgLumVertical.comp"}};
    shaders[0].constants.addConstant("GROUP_SIZE", uint32_t(_workAreaSize.y));
    verticalPass->setShaders(shaders);
    passes.push_back(std::move(verticalPass));
  }

  _techniqueConfigurator->setPasses(std::move(passes));
  _techniqueConfigurator->rebuildConfiguration();
}

void AvgLum::_updateBindings(CommandProducerCompute& commandProducer)
{
  _sourceImageBinding.setImage(_sourceImage);
  _invSourceSizeUniform.setValue(1.0f / glm::vec2(_sourceImage->extent()));

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
