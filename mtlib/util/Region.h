#pragma once

#include <glm/glm.hpp>

namespace mt
{
  //  Некоторая область 2D изображения, окна, экрана
  //  Задается координатами крайних пикселов, пренадлежащих этой области
  //  Если хотя бы одна координата maxCorner меньше, чем соответствующая
  //    координата minCorner, то бокс считается невалидным.
  struct Region
  {
    glm::uvec2 minCorner; //  Пиксель с наименьшими координатами
    glm::uvec2 maxCorner; //  Пиксель с наибольшими координатами

    //  Создает невалидный регион
    inline Region() noexcept;
    //  Создает регион по углам
    inline Region(unsigned int minX,
                  unsigned int minY,
                  unsigned int maxX,
                  unsigned int maxY) noexcept;
    //  Создает регион по координатам первого пикселя и размерам области
    inline Region(glm::uvec2 MinCorner, glm::uvec2 size) noexcept;
    //  Регион, начинающийся в нулевом пикселе и имеющий размер size
    inline Region(glm::uvec2 size) noexcept;
    Region(const Region&) noexcept = default;
    Region& operator = (const Region&) noexcept = default;
    ~Region() noexcept = default;

    inline bool operator == (const Region& other) const noexcept;

    inline bool valid() const noexcept;
    inline void invalidate() noexcept;

    //  Размер невалидного региона равен нулю
    inline glm::uvec2 size() const noexcept;
    inline unsigned int width() const noexcept;
    inline unsigned int height() const noexcept;

    inline void resize(glm::uvec2 newSize) noexcept;

    inline void move(glm::uvec2 newMinCorner) noexcept;

    //  Найти общие пиксели у двух регионов
    //  Если хотябы один из регионов невалидный, то результат так же будет
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
    return maxCorner.x >= minCorner.x && maxCorner.y >= minCorner.y;
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
