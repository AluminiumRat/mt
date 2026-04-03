#pragma once

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
    void updateReprojection(CommandProducerGraphic& commandProducer);

    inline void setBuffers(const ImageView& reprojectionBuffer);

  private:
    void createBuffers(CommandProducerGraphic& commandProducer);

  private:
    Device& _device;

    Technique _technique;
    TechniquePass& _updatePass;
    TechniquePass& _copyHistoryPass;
    Selection& _historyAvailableSelection;
    ResourceBinding& _reprojectionBufferBinding;
    ResourceBinding& _depthHistoryBinding;

    //  Буфер глубины с предыдущего кадра
    ConstRef<ImageView> _depthHistory;

    //  Размер сетки для вызова компьют шейдеров
    glm::uvec2 _gridSize;
  };

  inline void ReprojectionBufferUpdater::setBuffers(
                                            const ImageView& reprojectionBuffer)
  {
    if(_reprojectionBufferBinding.image() == &reprojectionBuffer) return;

    _reprojectionBufferBinding.setImage(&reprojectionBuffer);
    _gridSize = (glm::uvec2(reprojectionBuffer.extent()) + glm::uvec2(7)) / 8u;
  }
}