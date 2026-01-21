#pragma once

#include <hld/drawScene/Drawable.h>
#include <util/AABB.h>

namespace mt
{
  //  Рисуемый объект, у которого есть каие-то границы в пространстве
  class Drawable3D : public Drawable
  {
  public:
    explicit Drawable3D(DrawType drawType) noexcept;
    Drawable3D(const Drawable&) = delete;
    Drawable3D& operator = (const Drawable3D&) = delete;
    virtual ~Drawable3D() noexcept = default;

    inline const AABB& boundingBox() const noexcept;

    virtual Drawable3D* asDrawable3D() noexcept override;
    virtual const Drawable3D* asDrawable3D() const noexcept override;

  protected:
    virtual void setBoundingBox(const AABB& newBound) noexcept;

  private:
    AABB _boundingBox;
  };

  inline const AABB& Drawable3D::boundingBox() const noexcept
  {
    return _boundingBox;
  }
}