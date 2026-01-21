#pragma once

#include <glm/glm.hpp>

#include <util/AABB.h>
#include <util/Assert.h>

namespace mt
{
  class Plane
  {
  public:
    inline Plane() noexcept;
    inline explicit Plane(const glm::vec4& polynomial) noexcept;
    inline Plane(const glm::vec3& point, const glm::vec3& normal) noexcept;
    Plane(const Plane&) = default;
    Plane& operator = (const Plane&) = default;
    ~Plane() noexcept = default;

    inline const glm::vec4& polynomial() const noexcept;
    inline glm::vec3 normal() const noexcept;

    // Проекция начала координат на плоскость
    inline glm::vec3 originProjection() const noexcept;

    inline void setPolynomial(const glm::vec4& polynomial) noexcept;
    inline void setPointNormal( const glm::vec3& point,
                                glm::vec3 normal) noexcept;

    inline float signedDistance(const glm::vec3 point) const noexcept;

    //  Возвращает treue, если хотябы одна точка AABB находится либо на
    //  плоскости, либо с противоположной от нормали стороны (лежит под
    //  поверхностью)
    inline bool isBoxBelow(const AABB& box) const noexcept;

    //  Применить матрицу преобразования к плоскости
    //  Под капотом использует инвертирование-транспонирование матрицы
    inline void translate(const glm::mat4& matrix) noexcept;

    //  То же, что и translate, но использует внешнее
    //    инвертирование-транспонирование, что позволяет применять одну матрицу
    //    сразу к нескольким плоскостям.
    //  Также, если заранее известно, что матрица преобразования содержит только
    //    перемещение и вращение, то инверсию-транспонирование можно не
    //    использовать.
    inline void fastTranslate(const glm::mat4& inverseTransposeMatrix) noexcept;

  public:
    inline void _updateCachedValues() noexcept;

  private:
    glm::vec4 _polynomial;

    //  Эти значения - кэш для определения пересечения с AABB
    glm::vec3 _normalNormalPlus;  //  abs(normal) + normal
    glm::vec3 _normalNormalMinus; //  abs(normal) - normal
    float _doubleW;               //  2 * _polynomial.w
  };

  inline Plane::Plane() noexcept :
    _polynomial(0, 0, 1, 0),
    _normalNormalPlus(0, 0, 2),
    _normalNormalMinus(0, 0, 0),
    _doubleW(0)
  {
  }

  inline Plane::Plane(const glm::vec4& polynomial) noexcept :
    _polynomial(0, 0, 1, 0),
    _normalNormalPlus(0, 0, 2),
    _normalNormalMinus(0, 0, 0),
    _doubleW(0)
  {
    setPolynomial(polynomial);
  }

  inline Plane::Plane(const glm::vec3& point, const glm::vec3& normal) noexcept:
    _polynomial(0, 0, 1, 0),
    _normalNormalPlus(0, 0, 2),
    _normalNormalMinus( 0, 0, 0),
    _doubleW(0)
  {
    setPointNormal(point, normal);
  }

  inline glm::vec3 Plane::normal() const noexcept
  {
    return _polynomial;
  }

  inline glm::vec3 Plane::originProjection() const noexcept
  {
    glm::vec3 toPlaneDirection = -normal();
    return toPlaneDirection * _polynomial.w;
  }

  inline const glm::vec4& Plane::polynomial() const noexcept
  {
    return _polynomial;
  }

  inline void Plane::_updateCachedValues() noexcept
  {
    glm::vec3 normal = _polynomial;
    glm::vec3 absNormal = glm::abs(normal);
    _normalNormalPlus = absNormal + normal;
    _normalNormalMinus = absNormal - normal;
    _doubleW = 2.0f * _polynomial.w;
  }

  inline void Plane::setPolynomial(const glm::vec4& polynomial) noexcept
  {
    MT_ASSERT( polynomial.x != 0.f || polynomial.y != 0.f || polynomial.z != 0.f);

    glm::vec3 normal(polynomial.x, polynomial.y, polynomial.z);
    _polynomial = polynomial / glm::length(normal);
    _updateCachedValues();
  }

  inline void Plane::setPointNormal(const glm::vec3& point,
                                    glm::vec3 normal) noexcept
  {
    MT_ASSERT(normal.x != 0.f || normal.y != 0.f || normal.z != 0.f);

    normal = glm::normalize(normal);
    float distance = glm::dot(normal, -point);

    _polynomial = glm::vec4(normal, distance);
    _updateCachedValues();
  }

  inline float Plane::signedDistance(const glm::vec3 point) const noexcept
  {
    float inverseNormalDistanceToNull = glm::dot(normal(), point);
    return _polynomial.w + inverseNormalDistanceToNull;
  }

  inline bool Plane::isBoxBelow(const AABB& box) const noexcept
  {
    return glm::dot(box.maxCorner, _normalNormalMinus) - 
                        glm::dot(box.minCorner, _normalNormalPlus) >= _doubleW;
  }

  inline void Plane::translate(const glm::mat4& matrix) noexcept
  {
    fastTranslate(glm::transpose(glm::inverse(matrix)));
  }

  inline void Plane::fastTranslate(
                              const glm::mat4& inverseTransposeMatrix) noexcept
  {
    setPolynomial(inverseTransposeMatrix * _polynomial);
  }
}