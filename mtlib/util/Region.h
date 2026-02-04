#pragma once

#include <glm/glm.hpp>

namespace mt
{
  //  Ќекотора€ область 2D изображени€, окна, экрана
  //  «адаетс€ координатами крайних пикселов, пренадлежащих этой области
  //  ≈сли хот€ бы одна координата maxCorner меньше, чем соответствующа€
  //    координата minCorner, то бокс считаетс€ невалидным.
  struct Region
  {
    glm::uvec2 minCorner; //  ѕиксель с наименьшими координатами
    glm::uvec2 maxCorner; //  ѕиксель с наибольшими координатами

    //  —оздает невалидный регион
    inline Region() noexcept;
    //  —оздает регион по углам
    inline Region(unsigned int minX,
                  unsigned int minY,
                  unsigned int maxX,
                  unsigned int maxY) noexcept;
    //  —оздает регион по координатам первого пиксел€ и размерам области
    inline Region(glm::uvec2 MinCorner, glm::uvec2 size) noexcept;
    //  –егион, начинающийс€ в нулевом пикселе и имеющий размер size
    inline Region(glm::uvec2 size) noexcept;
    Region(const Region&) noexcept = default;
    Region& operator = (const Region&) noexcept = default;
    ~Region() noexcept = default;

    inline bool operator == (const Region& other) const noexcept;

    inline bool valid() const noexcept;
    inline void invalidate() noexcept;

    //  –азмер невалидного региона равен нулю
    inline glm::uvec2 size() const noexcept;
    inline unsigned int width() const noexcept;
    inline unsigned int height() const noexcept;

    inline void resize(glm::uvec2 newSize) noexcept;

    inline void move(glm::uvec2 newMinCorner) noexcept;

    //  Ќайти общие пиксели у двух регионов
    //  ≈сли хот€бы один из регионов невалидный, то результат так же будет
    //  невалидным регионом
    inline Region intersection(const Region& other) noexcept;
  };

  inline Region::Region() noexcept :
    minCorner(1),
    maxCorner(0)
  {
  }
  
  inline Region::Region(unsigned int minX,
                        unsigned int minY,
                        unsigned int maxX,
                        unsigned int maxY) noexcept :
    minCorner(minX, minY),
    maxCorner(maxX, maxY)
  {
  }
  
  inline Region::Region(glm::uvec2 MinCorner, glm::uvec2 size) noexcept :
    minCorner(1),
    maxCorner(0)
  {
    if(size.x > 0 && size.y > 0)
    {
      minCorner = MinCorner;
      maxCorner = minCorner + size - glm::uvec2(1);
    }
  }

  inline Region::Region(glm::uvec2 size) noexcept :
    minCorner(1),
    maxCorner(0)
  {
    if(size.x > 0 && size.y > 0)
    {
      minCorner = glm::uvec2(0);
      maxCorner = size - glm::uvec2(1);
    }
  }

  inline bool Region::operator == (const Region& other) const noexcept
  {
    return minCorner == other.minCorner && maxCorner == other.maxCorner;
  }

  inline bool Region::valid() const noexcept
  {
    return maxCorner.x > minCorner.x && maxCorner.y > minCorner.y;
  }

  inline void Region::invalidate() noexcept
  {
    minCorner = glm::uvec2(1);
    maxCorner = glm::uvec2(0);
  }

  inline glm::uvec2 Region::size() const noexcept
  {
    if(!valid()) return glm::uvec2(0);
    return maxCorner - minCorner + glm::uvec2(1);
  }

  inline unsigned int Region::width() const noexcept
  {
    if(!valid()) return 0;
    return maxCorner.x - minCorner.x + 1;
  }

  inline unsigned int Region::height() const noexcept
  {
    if(!valid()) return 0;
    return maxCorner.y - minCorner.y + 1;
  }

  inline void Region::resize(glm::uvec2 newSize) noexcept
  {
    if(newSize == glm::uvec2(0)) invalidate();
    maxCorner = minCorner + newSize - glm::uvec2(1);
  }

  inline void Region::move(glm::uvec2 newMinCorner) noexcept
  {
    if(!valid()) return;
    glm::uvec2 almostSize = maxCorner - minCorner;
    minCorner = newMinCorner;
    maxCorner = minCorner + almostSize;
  }

  inline Region Region::intersection(const Region& other) noexcept
  {
    Region result;
    result.minCorner = glm::max(minCorner, other.minCorner);
    result.maxCorner = glm::min(maxCorner, other.maxCorner);
    return result;
  }
}
