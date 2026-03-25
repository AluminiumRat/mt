#pragma once

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class ImageView;

  //  Строит буфер репроекции из текущего кадра в предыдущий, используя линейный
  //    буферу глубини и информации о перемещении камеры.
  //  Буфер репроекции в xy каналах содержит смещение скрин спэйс координат
  //    точки, в z канале - ожидаемое смещение в линейном буфере глубины, в w
  //    канале - достоверность репроецкии (1 - уверены, что репроекция
  //    корректная, 0 - уверены, что репроекция не корректная, то есть точка
  //    не имеет истории на экране и появилась только в этом фрэйме)
  class ReprojectionBufferUpdater
  {
  public:
    explicit ReprojectionBufferUpdater(Device& device);
    ReprojectionBufferUpdater(const ReprojectionBufferUpdater&) = delete;
    ReprojectionBufferUpdater& operator = (
                                    const ReprojectionBufferUpdater&) = delete;
    ~ReprojectionBufferUpdater() noexcept = default;

    //  К моменту вызова reprojectionBuffer должен находиться в лэйауте
    //    VK_IMAGE_LAYOUT_GENERAL
    void updateReprojection(CommandProducerGraphic& commandProducer,
                            const ImageView& reprojectionBuffer);
  private:
    Ref<TechniqueConfigurator> _techniqueConfigurator;
    Technique _technique;
    TechniquePass& _pass;
    ResourceBinding& _reprojectionBufferBinding;
  };
}