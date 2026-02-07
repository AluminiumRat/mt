#pragma once

#include <optional>
#include <utility>
#include <vector>

#include <vkr/image/ImageAccess.h>

namespace mt
{
  class Image;

  //  Набор описаний одновременного доступа к нескольким Image. Используется
  //  для автоконтроля лэйаутов. Объединяет информацию о доступе для одних и
  //  тех же Image. Сортирует информацию в порядке возрастания адрессов Image.
  class ImageAccessSet
  {
  public:
    using AccessRecord = std::pair<const Image*, ImageAccess>;
    using AccessTable = std::vector<AccessRecord>;

  public:
    ImageAccessSet();
    ImageAccessSet(const ImageAccessSet&) = default;
    ImageAccessSet& operator = (const ImageAccessSet&) = default;
    ImageAccessSet(ImageAccessSet&&) noexcept = default;
    ImageAccessSet& operator = (ImageAccessSet&&) noexcept = default;
    ~ImageAccessSet() noexcept = default;

    //  Добавить доступ к уже имеющемуся набору
    //  Если image уже присутствует в наборе, то будет попытка объединить
    //    информацию о доступе к нему. Если объединить доступы невозможно,
    //    например из-за пересекающихся слайсов или достижения лимита слайсов,
    //    то вернет false.
    bool addImageAccess(const Image& image, const ImageAccess& access);

    //  Все доступы, хранящиеся в сете
    inline const AccessTable& accessTable() const noexcept;

    inline bool empty() const noexcept;
    inline void clear() noexcept;

    //  Объединить два набора в один
    //  Возвращает nullopt, если объединить невозможно, например из-за
    //    пересекающихся слайсов или достижения лимита слайсов.
    static std::optional<ImageAccessSet> merge( const ImageAccessSet& first,
                                                const ImageAccessSet& second);

  private:
    AccessTable _accessTable;
  };

  inline const ImageAccessSet::AccessTable&
                                ImageAccessSet::accessTable() const noexcept
  {
    return _accessTable;
  }

  inline bool ImageAccessSet::empty() const noexcept
  {
    return _accessTable.empty();
  }

  inline void ImageAccessSet::clear() noexcept
  {
    _accessTable.clear();
  }
};