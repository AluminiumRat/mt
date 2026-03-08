#include <stdexcept>

#include <glm/glm.hpp>

#include <util/Assert.h>
#include <vkr/accelerationStructure/BLAS.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/Device.h>

using namespace mt;

BLAS::BLAS( std::span<const BLASGeometry> geometry, const char* debugName) :
  _device(geometry[0].positions->device()),
  _debugName(debugName),
  _handle(VK_NULL_HANDLE),
  _deviceAddress(0),
  _geometry(geometry.begin(), geometry.end())
{
  try
  {
    _makeHandle();
  }
  catch(...)
  {
    _clear();
    throw;
  }
}

BLAS::~BLAS() noexcept
{
  _clear();
}

void BLAS::_clear() noexcept
{
  if(_handle != nullptr)
  {
    _device.extFunctions().vkDestroyAccelerationStructureKHR(_handle);
    _handle = VK_NULL_HANDLE;
  }
  _deviceAddress = 0;
  _blasBuffer = nullptr;
  _scratchBuffer = nullptr;
}

void BLAS::_makeHandle()
{
  std::vector<VkAccelerationStructureGeometryKHR> geometryInfo =
                                                makeBLASGeometryInfo(_geometry);
  MT_ASSERT(!geometryInfo.empty());

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometyInfo{};
  buildGeometyInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometyInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildGeometyInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometyInfo.geometryCount = (uint32_t)geometryInfo.size();
  buildGeometyInfo.pGeometries = geometryInfo.data();

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

  _scratchBuffer = new DataBuffer(
                        _device,
                        sizeInfo.buildScratchSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":BLASScratch").c_str());

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

  VkAccelerationStructureDeviceAddressInfoKHR blasInfo{};
  blasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  blasInfo.accelerationStructure = _handle;
  _deviceAddress = _device.extFunctions().
                          vkGetAccelerationStructureDeviceAddressKHR(&blasInfo);
  if(_deviceAddress == 0) throw std::runtime_error(_debugName + ": unbale to get BLAS's device address");
}

VkAccelerationStructureBuildSizesInfoKHR BLAS::_getSizeInfo(
          const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const
{
  std::vector<uint32_t> primitivesCount;
  for(const BLASGeometry& geometry : _geometry)
  {
    if(geometry.indices != nullptr)
    {
      primitivesCount.push_back((uint32_t)geometry.indexCount / 3);
    }
    else
    {
      primitivesCount.push_back((uint32_t)geometry.vertexCount / 3);
    }
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

void BLAS::build(CommandProducerCompute& producer)
{
  if(_scratchBuffer == nullptr) return;
  producer.buildBLAS(*this, *_scratchBuffer);
}
