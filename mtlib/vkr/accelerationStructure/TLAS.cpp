#include <vkr/accelerationStructure/TLAS.h>
#include <vkr/Device.h>
#include <vkr/ExtFunctions.h>

using namespace mt;

TLAS::TLAS( Device& device,
            const BLASInstances& instances,
            const char* debugName) :
  _device(device),
  _debugName(debugName),
  _blases(instances)
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

TLAS::~TLAS() noexcept
{
  _clear();
}

void TLAS::_clear() noexcept
{
  if(_handle != nullptr)
  {
    _device.extFunctions().vkDestroyAccelerationStructureKHR(_handle);
    _handle = VK_NULL_HANDLE;
  }
  _tlasBuffer = nullptr;
  _scratchBuffer = nullptr;
  _geometryBuffer = nullptr;
}

void TLAS::_makeHandle()
{
  std::vector<VkAccelerationStructureInstanceKHR> instances =
                                                            makeInstancesInfo();
  VkAccelerationStructureGeometryDataKHR geometryData{};
  geometryData.instances = VkAccelerationStructureGeometryInstancesDataKHR{};
  geometryData.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  geometryData.instances.arrayOfPointers = VK_FALSE;
  geometryData.instances.data.hostAddress = instances.data();

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry = geometryData;

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometyInfo{};
  buildGeometyInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometyInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildGeometyInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometyInfo.geometryCount = 1;
  buildGeometyInfo.pGeometries = &geometry;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo =
                                                _getSizeInfo(buildGeometyInfo);
  _tlasBuffer = new DataBuffer(
                        _device,
                        sizeInfo.accelerationStructureSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":TLAS").c_str());

  _scratchBuffer = new DataBuffer(
                        _device,
                        sizeInfo.buildScratchSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":TLASScratch").c_str());

  _geometryBuffer = new DataBuffer(
                          _device,
                          instances.size() * sizeof(instances[0]),
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                          0,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          (_debugName + ":TLASGeometry").c_str());

  VkAccelerationStructureCreateInfoKHR createTLASInfo{};
  createTLASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  createTLASInfo.buffer = _tlasBuffer->handle();
  createTLASInfo.size = _tlasBuffer->size();
  createTLASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  if(_device.extFunctions().vkCreateAccelerationStructureKHR(
                                                        &createTLASInfo,
                                                        &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error(_debugName + ": unbale to create TLAS");
  }
}

std::vector<VkAccelerationStructureInstanceKHR> TLAS::makeInstancesInfo() const
{
  std::vector<VkAccelerationStructureInstanceKHR> instances;
  instances.reserve(_blases.size());
  for(const BLASInstance& blas : _blases)
  {
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = blas.transform[0][0];
    instance.transform.matrix[1][0] = blas.transform[0][1];
    instance.transform.matrix[2][0] = blas.transform[0][2];
    instance.transform.matrix[0][1] = blas.transform[1][0];
    instance.transform.matrix[1][1] = blas.transform[1][1];
    instance.transform.matrix[2][1] = blas.transform[1][2];
    instance.transform.matrix[0][2] = blas.transform[2][0];
    instance.transform.matrix[1][2] = blas.transform[2][1];
    instance.transform.matrix[2][2] = blas.transform[2][2];
    instance.transform.matrix[0][3] = blas.transform[3][0];
    instance.transform.matrix[1][3] = blas.transform[3][1];
    instance.transform.matrix[2][3] = blas.transform[3][2];
    instance.flags = 0xFF;
    instance.accelerationStructureReference = blas.blas->deviceAddress();
    instances.push_back({instance});
  }
  return instances;
}

VkAccelerationStructureBuildSizesInfoKHR TLAS::_getSizeInfo(
          const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const
{
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  uint32_t blasCount = (uint32_t)_blases.size();
  VkResult result =
    _device.extFunctions().vkGetAccelerationStructureBuildSizesKHR(
                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                &geometryInfo,
                                &blasCount,
                                &sizeInfo);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Unable to call vkGetAccelerationStructureBuildSizesKHR. Check the extension VK_KHR_acceleration_structure.");
  }
  return sizeInfo;
}

void TLAS::build(CommandProducerCompute& producer)
{
  if(_scratchBuffer == nullptr) return;

  producer.buildTLAS(*this, *_geometryBuffer, *_scratchBuffer);
}
