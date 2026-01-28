#pragma once

#include <util/Ref.h>
#include <vkr/image/Image.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  //  Создает текстуру с полным набором мипов из hdr буфера
  class BrightnessPyramid
  {
  public:
    BrightnessPyramid(Device& device);
    BrightnessPyramid(const BrightnessPyramid&) = delete;
    BrightnessPyramid& operator = (const BrightnessPyramid&) = delete;
    ~BrightnessPyramid() noexcept = default;

    //  Построить пирамиду по hdr буферу
    //  hdrBuffer должен либо быть в лэйауте
    //    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, либо быть со включенным
    //    автоконтролем лэйаутов
    void update(CommandProducerGraphic& commandProducer, Image& hdrBuffer);

    //  image, в котором была построена пирамида
    //  Возвращает nullptr, если update ещё не был вызван, либо последний
    //    вызов закончисля исключением
    //  Возвращает Image с выключенным автоконтролем и в лэйауте
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    inline Image* pyramidImage() const noexcept;

  private:
    void _createImage(Image& hdrBuffer);
    void _fillImage(CommandProducerGraphic& commandProducer, Image& hdrBuffer);

  private:
    Device& _device;
    Ref<Image> _pyramidImage;
  };

  inline Image* BrightnessPyramid::pyramidImage() const noexcept
  {
    return _pyramidImage.get();
  }
}