#include <stdexcept>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <vkr/Device.h>

#include <BLASAsset.h>

using namespace mt;

BLASAsset::BLASAsset( const std::vector<Geometry>& geometryData,
                      CommandProducerGraphic& producer,
                      const char* debugName) :
  _device(geometryData[0].positions->device()),
  _debugName(debugName),
  _handle(VK_NULL_HANDLE),
  _geometry(geometryData)
{
  try
  {
    _makeHandle(producer);
  }
  catch(...)
  {
    _clear();
    throw;
  }
}

BLASAsset::~BLASAsset() noexcept
{
  _clear();
}

void BLASAsset::_clear() noexcept
{
  if(_handle != nullptr)
  {
    _device.extFunctions().vkDestroyAccelerationStructureKHR(_handle);
    _handle = VK_NULL_HANDLE;
  }
}

void BLASAsset::_makeHandle(CommandProducerGraphic& producer)
{
  std::vector<VkAccelerationStructureGeometryKHR> geometry =
                                                            _makeGeometryInfo();
  MT_ASSERT(!geometry.empty());

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometyInfo{};
  buildGeometyInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometyInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildGeometyInfo.geometryCount = (uint32_t)geometry.size();
  buildGeometyInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometyInfo.pGeometries = geometry.data();

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo =
                                                _getSizeInfo(buildGeometyInfo);

  _blasBuffer = new DataBuffer(
                        _device,
                        sizeInfo.accelerationStructureSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":BLAS").c_str());

  ConstRef<DataBuffer> scratchBuffer( new DataBuffer(
                        _device,
                        sizeInfo.buildScratchSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":Scratch").c_str()));

  VkAccelerationStructureCreateInfoKHR createBLASInfo{};
  createBLASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createBLASInfo.buffer = _blasBuffer->handle();
  createBLASInfo.size = _blasBuffer->size();
  createBLASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

  if(_device.extFunctions().vkCreateAccelerationStructureKHR(
                                                        &createBLASInfo,
                                                        &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error(_debugName + ": unbale to create BLAS");
  }

  buildGeometyInfo.dstAccelerationStructure = _handle;
  buildGeometyInfo.scratchData.deviceAddress = scratchBuffer->deviceAddress();
}

std::vector<VkAccelerationStructureGeometryKHR>
                                            BLASAsset::_makeGeometryInfo() const
{
  std::vector<VkAccelerationStructureGeometryKHR> geometry;
  geometry.resize(_geometry.size());

  for(int setIndex = 0; setIndex < _geometry.size(); setIndex++)
  {
    const DataBuffer* positions = _geometry[setIndex].positions.get();
    MT_ASSERT(positions != nullptr);
    const DataBuffer* indices = _geometry[setIndex].indices.get();

    VkAccelerationStructureGeometryDataKHR geometryData;
    geometryData.triangles = {};
    geometryData.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometryData.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometryData.triangles.vertexData.deviceAddress =
                                                    positions->deviceAddress();
    geometryData.triangles.vertexStride = sizeof(glm::vec3);
    geometryData.triangles.maxVertex =
                                  (uint32_t)_geometry[setIndex].vertexCount - 1;
    if(indices != nullptr)
    {
      geometryData.triangles.indexType = VK_INDEX_TYPE_UINT32;
      geometryData.triangles.indexData.deviceAddress = indices->deviceAddress();
    }
    else
    {
      geometryData.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
    }

    geometry[setIndex] = {};
    geometry[setIndex].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry[setIndex].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry[setIndex].geometry = geometryData;
    geometry[setIndex].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  }

  return geometry;
}

VkAccelerationStructureBuildSizesInfoKHR BLASAsset::_getSizeInfo(
          const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const
{
  std::vector<uint32_t> primitivesCount;
  for(const Geometry& geometry : _geometry)
  {
    primitivesCount.push_back(uint32_t(geometry.indexCount) / 3);
  }

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  VkResult result =
    _device.extFunctions().vkGetAccelerationStructureBuildSizesKHR(
                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                &geometryInfo,
                                primitivesCount.data(),
                                &sizeInfo);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Unable to call vkGetAccelerationStructureBuildSizesKHR. Check the extension VK_KHR_acceleration_structure.");
  }
  return sizeInfo;
}
