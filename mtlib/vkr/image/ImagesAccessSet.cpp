#include <algorithm>

#include <util/Assert.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImagesAccessSet.h>

using namespace mt;

ImagesAccessSet::ImagesAccessSet()
{
  _accessTable.reserve(16);
}

bool ImagesAccessSet::addImageAccess( const Image& image,
                                      const ImageAccess& access)
{
  MT_ASSERT(image.isLayoutAutoControlEnabled());

  //  Ищем место, куда будем вставлять новый доступ, или запись в таблицу, с
  //  которой будем мержить новый доступ
  AccessTable::iterator insertPosition = _accessTable.begin();
  for(; insertPosition != _accessTable.end(); insertPosition++)
  {
    if(insertPosition->first >= &image) break;
  }

  if(insertPosition == _accessTable.end() || insertPosition->first != &image)
  {
    // Это новый image в таблице, просто вставляем новую запись
    _accessTable.insert(insertPosition, { &image, access });
    return true;
  }
  else
  {
    // Мы нашли дубликат Image-а, для начала проверяем, нельзя ли
    // переиспользовать уже используемые слайсы
    if(insertPosition->second.mergeNoBarriers(access)) return true;

    // Если переиспользовать не получается, то пытаемся добавить новый слайс
    // поверх уже существующих
    ImageAccess updatedAccess = insertPosition->second;
    if(updatedAccess.mergeWithBarriers(access) == ImageAccess::REUSE_SLICES)
    {
      insertPosition->second = updatedAccess;
      return true;
    }
    return false;
  }
}

std::optional<ImagesAccessSet> ImagesAccessSet::merge(
                                            const ImagesAccessSet& first,
                                            const ImagesAccessSet& second)
{
  if(second.empty()) return first;
  if(first.empty()) return second;

  // Резервируем место, куда будем объединять таблицы доступов
  ImagesAccessSet merged;
  merged._accessTable.reserve(first._accessTable.size() +
                                                    second._accessTable.size());

  // Объединяем 2 массива доступов, пользуясь тем, что они упорядоченные
  AccessTable::const_iterator firstIndex = first._accessTable.begin();
  AccessTable::const_iterator secondIndex = second._accessTable.begin();
  while(firstIndex != first._accessTable.end() &&
        secondIndex != second._accessTable.end())
  {
    const Image* firstImage = firstIndex->first;
    const Image* secondImage = firstIndex->first;
    if(firstImage < secondImage)
    {
      merged._accessTable.push_back(*firstIndex);
      firstIndex++;
    }
    else if(secondImage < firstImage)
    {
      merged._accessTable.push_back(*secondIndex);
      secondIndex++;
    }
    else
    {
      // Обнаружились доступы к одному Image-у. Пытаемся из объединить в один
      ImageAccess newAccess = firstIndex->second;
      const ImageAccess& secondAccess = secondIndex->second;

      // Для начала проверяем, нельзя ли переиспользовать слайсы
      if(!newAccess.mergeNoBarriers(secondAccess))
      {
        // Если переиспользовать не получается, то пытаемся добавить новый слайс
        // поверх уже существующих
        if(newAccess.mergeWithBarriers(secondAccess) !=
                                                      ImageAccess::REUSE_SLICES)
        {
          return std::nullopt;
        }
      }

      merged._accessTable.push_back({firstImage, newAccess});
      firstIndex++;
      secondIndex++;
    }
  }

  // Добираем необработанные хвосты
  for(; firstIndex != first._accessTable.end(); firstIndex++)
  {
    merged._accessTable.push_back(*firstIndex);
  }
  for (; secondIndex != second._accessTable.end(); secondIndex++)
  {
    merged._accessTable.push_back(*secondIndex);
  }

  return merged;
}
