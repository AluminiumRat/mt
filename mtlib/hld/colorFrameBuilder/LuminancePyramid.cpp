#include <hld/colorFrameBuilder/LuminancePyramid.h>
#include <hld/colorFrameBuilder/ColorFrameBuilder.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

LuminancePyramid::LuminancePyramid(Device& device) :
  _device(device)
{
}

void LuminancePyramid::update( CommandProducerGraphic& commandProducer,
                                Image& hdrBuffer)
{
  try
  {
    commandProducer.beginDebugLabel("LuminancePyramid");

    _createImage(hdrBuffer);
    _fillImage(commandProducer, hdrBuffer);

    commandProducer.endDebugLabel();
  }
  catch(...)
  {
    _pyramidImage = nullptr;
    throw;
  }
}

void LuminancePyramid::_createImage(Image& hdrBuffer)
{
  glm::uvec3 pyramidExtent = glm::max(glm::uvec3( hdrBuffer.extent().x / 2,
                                                  hdrBuffer.extent().y / 2,
                                                  1),
                                      glm::uvec3(1));
  if(_pyramidImage == nullptr || _pyramidImage->extent() != pyramidExtent)
  {
    uint32_t pyramidMips = Image::calculateMipNumber(pyramidExtent);
    _pyramidImage = new Image(_device,
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                VK_IMAGE_USAGE_SAMPLED_BIT,
                              0,
                              ColorFrameBuilder::hdrFormat,
                              pyramidExtent,
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              pyramidMips,
                              false,
                              "LuminancePyramid");
  }
}

void LuminancePyramid::_fillImage( CommandProducerGraphic& commandProducer,
                                    Image& hdrBuffer)
{
  //  Переводим всю пирамиду в VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  commandProducer.imageBarrier( *_pyramidImage,
                                ImageSlice(*_pyramidImage),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                0,
                                0,
                                0,
                                0);

  //  Блитим из HDR в нулевой мип пирамиды
  commandProducer.blitImage(hdrBuffer,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            0,
                            0,
                            glm::uvec3(0),
                            hdrBuffer.extent(),
                            *_pyramidImage,
                            VK_IMAGE_ASPECT_COLOR_BIT,
                            0,
                            0,
                            glm::uvec3(0),
                            _pyramidImage->extent(),
                            VK_FILTER_LINEAR);

  //  Последовательно блитим в менее детальные мипы, пока не дойдем до верха
  for(uint32_t srcMip = 0;
      srcMip < _pyramidImage->mipmapCount() - 1;
      srcMip++)
  {
    //  Меняем лэйаут мипа, из которого будем блитить, на
    //  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    commandProducer.imageBarrier( *_pyramidImage,
                                  ImageSlice( VK_IMAGE_ASPECT_COLOR_BIT,
                                              srcMip,
                                              1,
                                              0,
                                              1),
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_ACCESS_TRANSFER_WRITE_BIT,
                                  VK_ACCESS_TRANSFER_READ_BIT);

    commandProducer.blitImage(*_pyramidImage,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              0,
                              srcMip,
                              glm::uvec3(0),
                              _pyramidImage->extent(srcMip),
                              *_pyramidImage,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              0,
                              srcMip + 1,
                              glm::uvec3(0),
                              _pyramidImage->extent(srcMip + 1),
                              VK_FILTER_LINEAR);
  }

  //  Все мипы кроме верхнего находятся в лэйауте
  //  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. Меняем на
  //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  commandProducer.imageBarrier( *_pyramidImage,
                                ImageSlice(
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          0,
                                          _pyramidImage->mipmapCount() - 1,
                                          0,
                                          1),
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0,
                                VK_ACCESS_SHADER_READ_BIT);

  //  Верхний мип в лэйауте VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, меняем на
  //  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  commandProducer.imageBarrier( *_pyramidImage,
                                ImageSlice(
                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                          _pyramidImage->mipmapCount() - 1,
                                          1,
                                          0,
                                          1),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_SHADER_READ_BIT);
}
