#pragma once

#include <utility>
#include <vector>

#include <vkr/image/ImageAccess.h>

namespace mt
{
  class Image;

  //  Набор описаний одновременного доступа к нескольким Image. Используется
  //  для автоконтроля лэйаутов. Объединяет информацию о доступе для одних и
  //  тех же Image. Сортирует информацию в порядке возрастания адрессов Image.
  class ImagesAccessSet
  {
  public:
    using AccessRecord = std::pair<const Image*, ImageAccess>;
    using AccessTable = std::vector<AccessRecord>;

  public:
    ImagesAccessSet();
    ImagesAccessSet(const ImagesAccessSet&) = default;
    ImagesAccessSet& operator = (const ImagesAccessSet&) = default;
    ImagesAccessSet(ImagesAccessSet&&) noexcept = default;
    ImagesAccessSet& operator = (ImagesAccessSet&&) noexcept = default;
    ~ImagesAccessSet() noexcept = default;

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

  private:
    AccessTable _accessTable;
  };

  inline const ImagesAccessSet::AccessTable&
                                ImagesAccessSet::accessTable() const noexcept
  {
    return _accessTable;
  }

  inline bool ImagesAccessSet::empty() const noexcept
  {
    return _accessTable.empty();
  }

  inline void ImagesAccessSet::clear() noexcept
  {
    _accessTable.clear();
  }
};