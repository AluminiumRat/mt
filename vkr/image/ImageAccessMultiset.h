#pragma once

#include <array>

#include <vkr/image/ImagesAccessSet.h>

namespace mt
{
  //  Класс формирует единый ImageAccessSet из нескольких дочерних сетов.
  //  Используется в CommandProducer-ах для отслеживания доступа к Image-ам со
  //    стороны графического и компьют конвейеров
  class ImageAccessMultiset
  {
  public:
    static constexpr uint32_t maxSetsCount = 4;

  public:
    ImageAccessMultiset();
    ImageAccessMultiset(const ImageAccessMultiset&) = delete;
    ImageAccessMultiset& operator = (const ImageAccessMultiset&) = delete;
    ~ImageAccessMultiset() noexcept = default;

    //  childSet может быть nullptr, если вы хотите убрать дочерний сет из
    //    набора
    //  Если childSet не nullptr, то вызывающая сторона должна гарантировать
    //    что он будет существовать всё время, пока мультисет его использует.
    //    кроме того, childSet не должен изменяться за время его использования
    //    в мультисете.
    void setChild(const ImagesAccessSet* childSet, uint32_t index);
    inline const ImagesAccessSet& getMergedSet();

  private:
    void _mergeChilds();

  private:
    std::array<const ImagesAccessSet*, maxSetsCount> _childAccesses;
    ImagesAccessSet _cachedAccessSet;
    bool _needRecalculate;
  };

  inline const ImagesAccessSet& ImageAccessMultiset::getMergedSet()
  {
    if(_needRecalculate) _mergeChilds();
    return _cachedAccessSet;
  }
}