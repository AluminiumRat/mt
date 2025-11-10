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
