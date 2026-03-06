#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  //  Отдельная меш-сетка для построения BLAS
  struct BLASGeometry
  {
    //  Содержит положения вершин. По 3 float-а на вершину буз разрывов
    //  Не должен быть nullptr
    //  Должен быть создан с флагом VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
    ConstRef<DataBuffer> positions;
    //  Количество вершин в positions. Не должен быть 0
    size_t vertexCount;
    //  Сожержит 32-х разрядные индексы
    //  Может быть nullptr
    //  Если не nullptr, то должен быть создан с флагом
    //    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
    ConstRef<DataBuffer> indices;
    //  Количество индексов в indices. Если indices не nullptr, то не может
    //  быть 0. Если indeces == nullptr, то значение indexCount игнорируется
    size_t indexCount;
  };

  std::vector<VkAccelerationStructureGeometryKHR> makeBLASGeometryInfo(
                                        std::span<const BLASGeometry> geometry);
}