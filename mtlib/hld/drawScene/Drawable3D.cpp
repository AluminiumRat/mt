#include <hld/drawScene/Drawable3D.h>

using namespace mt;

Drawable3D::Drawable3D(DrawType drawType) noexcept :
  Drawable(drawType)
{
}

void Drawable3D::setBoundingBox(const AABB& newBound) noexcept
{
  _boundingBox = newBound;
}

Drawable3D* Drawable3D::asDrawable3D() noexcept
{
  return this;
}

const Drawable3D* Drawable3D::asDrawable3D() const noexcept
{
  return this;
}
