#pragma once

#include <algorithm>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <util/Sphere.h>

namespace mt
{
  //  Ориентированный по осям бокс
  //  Задается двумя точками - minCorner и maxCorner
  //  Если хотя бы одна координата maxCorner меньше, чем соответствующая у
  //    minCorner, то бокс считается невалидным. Валидность влияет на то, как
  //    будут просчитываться взаимодействия с другими объектами. Например,
  //    пересечения и расширение объектов
  struct AABB
  {
    glm::vec3 minCorner;
    glm::vec3 maxCorner;

    //  Создаем невалидный AABB
    inline AABB() noexcept;
    inline AABB(const glm::vec3& MinCorner, const glm::vec3 MaxCorner) noexcept;
    inline AABB(float minx,
                float miny,
                float minz,
                float maxx,
                float maxy,
                float maxz)  noexcept;
    AABB(const AABB&) noexcept = default;
    AABB& operator = (const AABB&) noexcept = default;
    ~AABB() noexcept = default;

    inline bool operator == (const AABB& other) const noexcept;

    inline bool valid() const noexcept;
    inline void invalidate() noexcept;

    //  Проверить, принадлежит ли точка AABB, влючая все его грани
    //  Если AABB невалидный, то всегда возвращает false
    inline bool isContainPoint(const glm::vec3& point) const noexcept;

    //  Проверить, принадлежит ли точка other этому AABB, влючая все грани
    //  Если хотя бы один из AABB невалидный, то всегда возвращает false
    inline bool isContainAABB(const AABB& other) const noexcept;

    //  Проверить, имеют ли боксы общие точки(включая все грани)
    //  Если хотя бы один из AABB невалидный, то всегда возвращает false
    inline bool intersected(const AABB& other) const noexcept;

    inline glm::vec3 size() const noexcept;
    inline float xSize() const noexcept;
    inline float ySize() const noexcept;
    inline float zSize() const noexcept;
    inline glm::vec3 center() const noexcept;

    //  Увеличить AABB так, чтобы он включал в себя point
    //  Если AABB невалидный, то он станет валидным, а его minCorner и maxCorner
    //    будут равны point
    inline void extend(const glm::vec3& point) noexcept;

    //  Увеличить AABB так, чтобы он включал в себя other
    //  Если other невалидный, то не делает ничего
    //  Если текущий AABB невалидный, то он станет равен other
    inline void extend(const AABB& other) noexcept;

    inline void scaleFromCenter(float scale) noexcept;

    inline AABB translated(const glm::mat4& translationMatrix) const noexcept;

    inline Sphere buildBoundingSphere() const noexcept;
  };

  inline AABB::AABB() noexcept :
    minCorner(0),
    maxCorner(-1)
  {
  }

  inline AABB::AABB(const glm::vec3& MinCorner,
                    const glm::vec3 MaxCorner) noexcept :
    minCorner(MinCorner),
    maxCorner(MaxCorner)
  {
  }

  inline AABB::AABB(float minx,
                    float miny,
                    float minz,
                    float maxx,
                    float maxy,
                    float maxz) noexcept :
    minCorner(minx, miny, minz),
    maxCorner(maxx, maxy, maxz)
  {
  }

  inline bool AABB::operator == (const AABB& other) const noexcept
  {
    return minCorner == other.minCorner && maxCorner == other.maxCorner;
  }

  inline bool AABB::valid() const noexcept
  {
    return  minCorner.x <= maxCorner.x &&
            minCorner.y <= maxCorner.y &&
            minCorner.z <= maxCorner.z;
  }

  inline void AABB::invalidate() noexcept
  {
    minCorner = glm::vec3(0);
    maxCorner = glm::vec3(-1);
  }

  inline bool AABB::isContainPoint(const glm::vec3& point) const noexcept
  {
    return  point.x >= minCorner.x &&
            point.x <= maxCorner.x &&
            point.y >= minCorner.y &&
            point.y <= maxCorner.y &&
            point.z >= minCorner.z &&
            point.z <= maxCorner.z;
  }

  inline bool AABB::isContainAABB(const AABB& other) const noexcept
  {
    if (!valid() || !other.valid()) return false;

    return  minCorner.x <= other.minCorner.x &&
            minCorner.y <= other.minCorner.y &&
            minCorner.z <= other.minCorner.z &&
            maxCorner.x >= other.maxCorner.x &&
            maxCorner.y >= other.maxCorner.y &&
            maxCorner.z >= other.maxCorner.z;
  }

  inline glm::vec3 AABB::size() const noexcept
  {
    return maxCorner - minCorner;
  }

  inline float AABB::xSize() const noexcept
  {
    return maxCorner.x - minCorner.x;
  }

  inline float AABB::ySize() const noexcept
  {
    return maxCorner.y - minCorner.y;
  }

  inline float AABB::zSize() const noexcept
  {
    return maxCorner.z - minCorner.z;
  }

  inline void AABB::extend(const glm::vec3& point) noexcept
  {
    if(!valid())
    {
      minCorner = point;
      maxCorner = point;
    }
    else
    {
      minCorner.x = std::min(minCorner.x, point.x);
      minCorner.y = std::min(minCorner.y, point.y);
      minCorner.z = std::min(minCorner.z, point.z);

      maxCorner.x = std::max(maxCorner.x, point.x);
      maxCorner.y = std::max(maxCorner.y, point.y);
      maxCorner.z = std::max(maxCorner.z, point.z);
    }
  }

  inline void AABB::extend(const AABB& other) noexcept
  {
    if (!other.valid()) return;
    extend(other.minCorner);
    extend(other.maxCorner);
  }

  inline Sphere AABB::buildBoundingSphere() const noexcept
  {
    if(!valid()) return Sphere();
    glm::vec3 sizeVector = maxCorner - minCorner;
    return Sphere((minCorner + maxCorner) / 2.f,
                  glm::length(sizeVector) / 2.f);
  }

  inline glm::vec3 AABB::center() const noexcept
  {
    return (minCorner + maxCorner) / 2.f;
  }

  inline void AABB::scaleFromCenter(float scale) noexcept
  {
    if(!valid()) return;
    glm::vec3 diagonal = maxCorner - minCorner;
    diagonal *= scale;
    glm::vec3 halfDiagonal = diagonal / 2.f;
    glm::vec3 theCenter = center();
    minCorner = theCenter - halfDiagonal;
    maxCorner = theCenter + halfDiagonal;
  }

  inline AABB AABB::translated(
                              const glm::mat4& translationMatrix) const noexcept
  {
    if(!valid()) return AABB();

    AABB result;
    result.extend(translationMatrix * glm::vec4(minCorner, 1.f));
    result.extend(translationMatrix * glm::vec4(minCorner.x,
                                                minCorner.y,
                                                maxCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(minCorner.x,
                                                maxCorner.y,
                                                minCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(minCorner.x,
                                                maxCorner.y,
                                                maxCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(maxCorner.x,
                                                minCorner.y,
                                                minCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(maxCorner.x,
                                                minCorner.y,
                                                maxCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(maxCorner.x,
                                                maxCorner.y,
                                                minCorner.z,
                                                1.f));
    result.extend(translationMatrix * glm::vec4(maxCorner, 1.f));

    return result;
  }

  inline bool AABB::intersected(const AABB& other) const noexcept
  {
    if (!valid()) return false;
    if (!other.valid()) return false;

    if(maxCorner.x < other.minCorner.x ||
                                  minCorner.x > other.maxCorner.x) return false;
    if(maxCorner.y < other.minCorner.y ||
                                  minCorner.y > other.maxCorner.y) return false;
    if(maxCorner.z < other.minCorner.z ||
                                  minCorner.z > other.maxCorner.z) return false;
    return true;
  }
}