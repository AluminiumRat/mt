#pragma once

#include <glm/glm.hpp>

#include <util/Ref.h>
#include <vkr/accelerationStructure/BLAS.h>

namespace mt
{
  struct BLASInstance
  {
    ConstRef<BLAS> blas;
    glm::mat4 transform = glm::mat4(1);

    //  Блок данных, напрямую взятый из VkAccelerationStructureInstanceKHR
    uint32_t instanceCustomIndex : 24 = 0;
    uint32_t mask : 8 = 0xFF;
    uint32_t instanceShaderBindingTableRecordOffset : 24 = 0;
    VkGeometryInstanceFlagsKHR flags : 8 = 0;
  };
  using BLASInstances = std::vector<BLASInstance>;
}