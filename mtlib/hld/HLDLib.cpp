#include <hld/HLDLib.h>

using namespace mt;

HLDLib* HLDLib::_instance = nullptr;

HLDLib::HLDLib()
{
  MT_ASSERT(_instance == nullptr);

  _instance = this;
}

HLDLib::~HLDLib()
{
  _instance = nullptr;
}