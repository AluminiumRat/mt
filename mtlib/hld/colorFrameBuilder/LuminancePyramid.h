#pragma once

#include <util/Ref.h>
#include <vkr/image/Image.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  //  Создает текстуру с полным набором мипов из hdr буфера
  class LuminancePyramid
  {
  public:
    LuminancePyramid(Device& device);
    LuminancePyramid(const LuminancePyramid&) = delete;
    LuminancePyramid& operator = (const LuminancePyramid&) = delete;
    ~LuminancePyramid() noexcept = default;

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

  inline Image* LuminancePyramid::pyramidImage() const noexcept
  {
    return _pyramidImage.get();
  }
}