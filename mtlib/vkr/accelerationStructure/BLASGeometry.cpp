#include <util/Assert.h>
#include <vkr/accelerationStructure/BLASGeometry.h>

std::vector<VkAccelerationStructureGeometryKHR> mt::makeBLASGeometryInfo(
                                        std::span<const BLASGeometry> geometry)
{
  std::vector<VkAccelerationStructureGeometryKHR> geometryInfo;
  geometryInfo.resize(geometry.size());

  for(int setIndex = 0; setIndex < geometry.size(); setIndex++)
  {
    const DataBuffer* positions = geometry[setIndex].positions.get();
    MT_ASSERT(positions != nullptr);
    const DataBuffer* indices = geometry[setIndex].indices.get();

    VkAccelerationStructureGeometryDataKHR geometryData;
    geometryData.triangles = {};
    geometryData.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometryData.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometryData.triangles.vertexData.deviceAddress =
                                                    positions->deviceAddress();
    geometryData.triangles.vertexStride = 3 * sizeof(float);
    geometryData.triangles.maxVertex =
                                  (uint32_t)geometry[setIndex].vertexCount - 1;
    if(indices != nullptr)
    {
      geometryData.triangles.indexType = VK_INDEX_TYPE_UINT32;
      geometryData.triangles.indexData.deviceAddress = indices->deviceAddress();
    }
    else
    {
      geometryData.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
    }

    geometryInfo[setIndex] = {};
    geometryInfo[setIndex].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometryInfo[setIndex].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometryInfo[setIndex].geometry = geometryData;
    geometryInfo[setIndex].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  }

  return geometryInfo;
}