#include <hld/colorFrameBuilder/ColorFrameCommonSet.h>
#include <hld/HLDLib.h>
#include <technique/CommonSetRegistry.h>

using namespace mt;

HLDLib* HLDLib::_instance = nullptr;

HLDLib::HLDLib() :
  _drawCommandGroupCount(DrawCommand::noGroup + 1)
{
  MT_ASSERT(_instance == nullptr);
  _instance = this;

  CommonSetRegistry::registerSet( ColorFrameCommonSet::setName,
                                  ColorFrameCommonSet::bindings);
}

HLDLib::~HLDLib()
{
  _instance = nullptr;
}