#include <util/Abort.h>
#include <util/Assert.h>
#include <vkr/image/ImageAccessMultiSet.h>

using namespace mt;

ImageAccessMultiset::ImageAccessMultiset() :
  _needRecalculate(false)
{
  for(uint32_t i = 0; i < maxSetsCount; i++)
  {
    _childAccesses[i] = nullptr;
  }
}

void ImageAccessMultiset::setChild( const ImagesAccessSet* childSet,
                                    uint32_t index) noexcept
{
  MT_ASSERT(index < maxSetsCount);

  if( _childAccesses[index] != nullptr &&
      !_childAccesses[index]->empty())
  {
    _needRecalculate = true;
  }

  _childAccesses[index] = childSet;

  if(childSet != nullptr && !childSet->empty()) _needRecalculate = true;
}

void ImageAccessMultiset::_mergeChilds()
{
  _cachedAccessSet.clear();
  for(uint32_t i = 0; i < maxSetsCount; i++)
  {
    if(_childAccesses[i] != nullptr)
    {
      std::optional<ImagesAccessSet> mergedSet =
                  ImagesAccessSet::merge(_cachedAccessSet, *_childAccesses[i]);
      if(!mergedSet.has_value()) Abort("Unable to merge access sets");
      _cachedAccessSet = std::move(*mergedSet);
    }
  }
}
