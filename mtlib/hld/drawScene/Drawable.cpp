#include <hld/drawScene/Drawable.h>

using namespace mt;

Drawable::Drawable(DrawType drawType) noexcept :
  _drawType(drawType)
{
}

void Drawable::draw(const FrameContext& frameContext) const
{
}

void Drawable::addToCommandList(DrawCommandList& commandList,
                                const FrameContext& frameContext) const
{
}
