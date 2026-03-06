#include <BLASAsset.h>

using namespace mt;

BLASAsset::BLASAsset(const std::vector<Geometry>& geometryData) :
  _device(geometryData[0].positions->device()),
  _buffers(geometryData)
{
}

BLASAsset::~BLASAsset() noexcept
{
  _clear();
}

void BLASAsset::_clear() noexcept
{
}
