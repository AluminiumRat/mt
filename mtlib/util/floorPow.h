#pragma once

#include <bit>

#include <glm/glm.hpp>

namespace mt
{
  //  Найти ближайшую снизу степень двойки
  inline glm::uvec2 floorPow(glm::uvec2 sourceSize) noexcept
  {
    glm::uvec2 sizeExp( std::bit_width(sourceSize.x) - 1,
                        std::bit_width(sourceSize.y) - 1);
    return glm::uvec2(1 << sizeExp.x, 1 << sizeExp.y);
  }
}