#include <hld/colorFrameBuilder/HdrReprojector.h>
#include <util/gaussian.h>
#include <vkr/queue/CommandProducerCompute.h>

using namespace mt;

HdrReprojector::HdrReprojector(Device& device) :
  _device(device),
  _reprojectTechnique(device, "ssr/hdrReprojector.tch"),
  _reprojectPass(_reprojectTechnique.getOrCreatePass("ReprojectPass")),
  _prevHdrBinding(_reprojectTechnique.getOrCreateResourceBinding("prevHDR")),
  _reprojectedHdrBinding(
          _reprojectTechnique.getOrCreateResourceBinding("reprojectedHdrOut")),
  _targetSizeUniform(
                  _reprojectTechnique.getOrCreateUniform("params.targetSize")),
  _targetMipCountUniform(
              _reprojectTechnique.getOrCreateUniform("params.targetMipCount")),
  _baseMipSelection(_reprojectTechnique.getOrCreateSelection("BASE_MIP")),
  _reprojectGridSize(1),
  _filterTechnique(device, "ssr/filter.tch"),
  _filterHorizontalPass(_filterTechnique.getOrCreatePass("HorizontalPass")),
  _filterVerticalPass(_filterTechnique.getOrCreatePass("VerticalPass")),
  _filterSourceBinding(
                    _filterTechnique.getOrCreateResourceBinding("sourceImage")),
  _filterTargetBinding(
                    _filterTechnique.getOrCreateResourceBinding("targetImage")),
  _horizontalFilterCore(_filterTechnique.getOrCreateUniform("params.horizontalCore")),
  _verticalFilterCore(_filterTechnique.getOrCreateUniform("params.verticalCore")),
  _filterGridSize(1)
{
}

void HdrReprojector::_calculateFilterCore()
{
  MT_ASSERT(_prevHdrBinding.image() != nullptr);
  MT_ASSERT(_reprojectedHdr != nullptr);

  //  Вычисляем ядро для фильтра Гаусса 3х3 пикселя.
  //  Так как оригинальный hdr буфер и репроецированный буфер могут иметь разное
  //    соотношение сторон, то необходимо сделать разные ядра для вертикального
  //    и горизонтального прохода.
  //  Подход заключается в том, чтобы по одной стороне сделать ширину фильтра
  //    в 4 сигмы (2 сигмы в каждую сторону). Для другой стороны ширина в сигмах
  //    будет больше, чтобы компенсировать разницу в соотношении сторон
  glm::uvec2 originalBufferSize = _prevHdrBinding.image()->extent();
  float originalAspectRatio = (float)originalBufferSize.x /
                                                          originalBufferSize.y;
  glm::uvec2 reprojectedBufferSize = _reprojectedHdr->extent();
  float reprojectedAspectRatio = (float)reprojectedBufferSize.x /
                                                        reprojectedBufferSize.y;
  float sigmaRatio = reprojectedAspectRatio / originalAspectRatio;

  //  Вычисляем ядро фильтра для ширины в 4 сигмы
  float sigma = 1.5f / 2.0f;
  glm::vec2 longFilerCore{gaussianIntegral(-0.5f, 0.5f, 100, sigma),
                          gaussianIntegral(0.5f, 1.5f, 100, sigma)};
  longFilerCore = longFilerCore / (longFilerCore[0] + 2.0f * longFilerCore[1]);

  //  Ядро фильтра с урезанной сигмой
  float shortSigma = sigmaRatio < 1.0f ?  sigma * sigmaRatio :
                                          sigma / sigmaRatio;
  glm::vec2 shortFilerCore{ gaussianIntegral(-0.5f, 0.5f, 100, shortSigma),
                            gaussianIntegral(0.5f, 1.5f, 100, shortSigma) };
  shortFilerCore = shortFilerCore /
                                (shortFilerCore[0] + 2.0f * shortFilerCore[1]);

  //  Выбираем, на какое направление какое едро отправить
  if(sigmaRatio > 1.0f)
  {
    _verticalFilterCore.setValue(shortFilerCore);
    _horizontalFilterCore.setValue(longFilerCore);
  }
  else
  {
    _verticalFilterCore.setValue(longFilerCore);
    _horizontalFilterCore.setValue(shortFilerCore);
  }
}

void HdrReprojector::_createPingpongBuffer(CommandProducerCompute& producer)
{
  _pinpongBuffer = ConstRef(new Image(_device,
                                      VK_IMAGE_TYPE_2D,
                                      VK_IMAGE_USAGE_STORAGE_BIT,
                                      0,
                                      _reprojectedHdr->format(),
                                      _reprojectedHdr->extent(),
                                      _reprojectedHdr->samples(),
                                      1,
                                      _reprojectedHdr->mipmapCount(),
                                      false,
                                      "HdrReprojectorPingpong"));
  producer.initLayout(*_pinpongBuffer, VK_IMAGE_LAYOUT_GENERAL);

  _pinpongViews.clear();
  for(uint32_t mip = 0; mip < _pinpongBuffer->mipmapCount(); mip++)
  {
    _pinpongViews.push_back(
                    ConstRef(new ImageView( *_pinpongBuffer,
                                            ImageSlice(
                                                VK_IMAGE_ASPECT_COLOR_BIT,
                                                mip),
                                            VK_IMAGE_VIEW_TYPE_2D)));
  }
}

void HdrReprojector::make(CommandProducerCompute& producer)
{
  MT_ASSERT(_reprojectedHdr != nullptr);

  if(_pinpongBuffer == nullptr) _createPingpongBuffer(producer);

  _makeMips(producer, "0", _reprojectGridSize);
  producer.imageBarrier(*_reprojectedHdr,
                        ImageSlice( *_reprojectedHdr,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    5),
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_SHADER_READ_BIT);

  _filterHorizontal(producer, 0, _filterGridSize);

  if(_reprojectedHdr->mipmapCount() > 5)
  {
    _makeMips(producer, "5", glm::max(_reprojectGridSize / 16u, 1u));
    producer.imageBarrier(*_reprojectedHdr,
                          ImageSlice( *_reprojectedHdr,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      5,
                                      5),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);
  }
  producer.imageBarrier(*_pinpongBuffer,
                        ImageSlice( *_pinpongBuffer,
                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    5),
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_ACCESS_SHADER_WRITE_BIT,
                        VK_ACCESS_SHADER_READ_BIT);

  _filterVertical(producer, 0, _filterGridSize);
  if(_reprojectedHdr->mipmapCount() > 5)
  {
    _filterHorizontal(producer, 5, glm::max(_filterGridSize / 16u, 1u));
    producer.imageBarrier(*_pinpongBuffer,
                          ImageSlice( *_pinpongBuffer,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      5,
                                      5),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);
    _filterVertical(producer, 5, glm::max(_filterGridSize / 16u, 1u));
  }
}

void HdrReprojector::_makeMips( CommandProducerCompute& producer,
                                const char* baseMipValue,
                                glm::uvec2 gridSize)
{
  _baseMipSelection.setValue(baseMipValue);
  Technique::BindCompute bind(_reprojectTechnique, _reprojectPass, producer);
  MT_ASSERT(bind.isValid());
  producer.dispatch(gridSize);
}

void HdrReprojector::_filterHorizontal( CommandProducerCompute& producer,
                                        uint32_t baseMip,
                                        glm::uvec2 gridSize)
{
  uint32_t endMip = std::min(baseMip + 5, _reprojectedHdr->mipmapCount());
  for(uint32_t mip = baseMip; mip < endMip; mip++)
  {
    _filterSourceBinding.setImage(_reprojectedHdrViews[mip]);
    _filterTargetBinding.setImage(_pinpongViews[mip]);
    Technique::BindCompute bind(_filterTechnique, _filterHorizontalPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(gridSize);
    gridSize = glm::max(gridSize / 2u, 1u);
  }
}

void HdrReprojector::_filterVertical( CommandProducerCompute& producer,
                      uint32_t baseMip,
                      glm::uvec2 gridSize)
{
  uint32_t endMip = std::min(baseMip + 5, _reprojectedHdr->mipmapCount());
  for(uint32_t mip = baseMip; mip < endMip; mip++)
  {
    _filterSourceBinding.setImage(_pinpongViews[mip]);
    _filterTargetBinding.setImage(_reprojectedHdrViews[mip]);
    Technique::BindCompute bind(_filterTechnique, _filterVerticalPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(gridSize);
    gridSize = glm::max(gridSize / 2u, 1u);
  }
}
