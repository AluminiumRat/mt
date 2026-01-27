#include <hld/drawScene/Drawable.h>

using namespace mt;

Drawable::Drawable(DrawType drawType) noexcept :
  _drawType(drawType)
{
}

void Drawable::draw(CommandProducerGraphic& commandProducer,
                    const FrameBuildContext& frameContext,
                    StageIndex stage,
                    const void* extraData) const
{
}

void Drawable::addToCommandList(DrawCommandList& commandList,
                                const FrameBuildContext& frameContext,
                                StageIndex stage,
                                const void* extraData) const
{
}

Drawable3D* Drawable::asDrawable3D() noexcept
{
  return nullptr;
}

const Drawable3D* Drawable::asDrawable3D() const noexcept
{
  return nullptr;
}
