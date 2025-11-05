#include <algorithm>

#include <vkr/queue/ImagesAccessSet.h>
#include <vkr/Image.h>

using namespace mt;

ImagesAccessSet::ImagesAccessSet()
{
  _accessTable.reserve(16);
}

bool ImagesAccessSet::addImageAccess(
                                const Image& image, const ImageAccess& access)
{
  MT_ASSERT(image.isLayoutAutoControlEnabled());

  if(_accessTable.empty())
  {
    _accessTable.push_back({&image, access });
    return true;
  }

  AccessTable::iterator insertPosition = _accessTable.begin();
  for(; insertPosition != _accessTable.end(); insertPosition++)
  {
    if(insertPosition->first >= &image) break;
  }

  if(insertPosition == _accessTable.end() || insertPosition->first != &image)
  {
    _accessTable.insert(insertPosition, { &image, access });
    return true;
  }
  else
  {
    return insertPosition->second.mergeNoBarriers(access);
  }
}
