#pragma once

#include <glm/glm.hpp>

#include <util/Ref.h>
#include <vkr/accelerationStructure/BLAS.h>

namespace mt
{
  struct BLASInstance
  {
    ConstRef<BLAS> blas;
    glm::mat4 transform;
  };
  using BLASInstances = std::vector<BLASInstance>;
}