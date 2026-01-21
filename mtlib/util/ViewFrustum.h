#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <util/Plane.h>
#include <util/Sphere.h>

namespace mt
{
  class ViewFrustum
  {
  public:
    enum Face
    {
      FACE_RIGHT = 0,
      FACE_LEFT = 1,
      FACE_UP = 2,
      FACE_DOWN = 3,
      FACE_NEAR = 4,
      FACE_FAR = 5
    };

  public:
    ViewFrustum() noexcept;
    explicit ViewFrustum(const glm::mat4& inverseViewProjectionMatrix) noexcept;
    ViewFrustum(const ViewFrustum&) = default;
    ViewFrustum& operator = (const ViewFrustum&) = default;
    ~ViewFrustum() = default;

    void setInverseViewProjectionMatrix(const glm::mat4& matrix) noexcept;
  
    inline const Plane& face(Face face) const noexcept;

    //  Проверить, есть ли пересечение со сферой
    inline bool intersect(const Sphere& sphere) const noexcept;

    //  Проверить, есть ли пересечение с AABB
    inline bool intersect(const AABB& box) const noexcept;

    //  Применить матрицу преобразования
    //  Под капотом использует инвертирование-транспонирование матрицы
    inline void translate(const glm::mat4& matrix) noexcept;

    //  То же, что и translate, но использует внешнее
    //    инвертирование-транспонирование
    //  Также, если заранее известно, что матрица преобразования содержит только
    //    перемещение и вращение, то инверсию-транспонирование можно не
    //    использовать.
    inline void fastTranslate(const glm::mat4& inverseTransposeMatrix) noexcept;

  private:
    Plane _faces[6];
  };

  inline const Plane& ViewFrustum::face(Face face) const noexcept
  {
    return _faces[int(face)];
  }

  inline bool ViewFrustum::intersect(const Sphere& sphere) const noexcept
  {
    if(!sphere.valid()) return false;

    for(const Plane& face : _faces)
    {
      if (face.signedDistance(sphere.center) > sphere.radius) return false;
    }

    return true;
  }

  inline bool ViewFrustum::intersect(const AABB& box) const noexcept
  {
    if(!box.valid()) return false;

    for (const Plane& face : _faces)
    {
      if (!face.isBoxBelow(box)) return false;
    }

    return true;
  }

  inline void ViewFrustum::translate(const glm::mat4& matrix) noexcept
  {
    glm::mat4 inverseTranspose = glm::transpose(glm::inverse(matrix));
    for (Plane& face : _faces) face.fastTranslate(inverseTranspose);
  }

  inline void ViewFrustum::fastTranslate(
                              const glm::mat4& inverseTransposeMatrix) noexcept
  {
    for (Plane& face : _faces) face.fastTranslate(inverseTransposeMatrix);
  }
}