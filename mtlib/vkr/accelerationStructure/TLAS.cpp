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
  VkAccelerationStructureGeometryDataKHR geometryData{};
  geometryData.instances = VkAccelerationStructureGeometryInstancesDataKHR{};
  geometryData.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  geometryData.instances.arrayOfPointers = VK_FALSE;

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry = geometryData;

  VkAccelerationStructureBuildSizesInfoKHR createSizes =
                                          _getCreateSizeInfo(geometry);
  _tlasBuffer = new DataBuffer(
                        _device,
                        createSizes.accelerationStructureSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":TLAS").c_str());

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

  VkAccelerationStructureBuildSizesInfoKHR updateSizes =
                                                  _getUpdateSizeInfo(geometry);
  _scratchBuffer = new DataBuffer(
                        _device,
                        std::max( createSizes.buildScratchSize,
                                  updateSizes.updateScratchSize),
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        0,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        (_debugName + ":TLASScratch").c_str());

  _geometryBuffer = new DataBuffer(
                          _device,
                          _blases.size() *
                                    sizeof(VkAccelerationStructureInstanceKHR),
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                          0,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          (_debugName + ":TLASGeometry").c_str());
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

    instance.instanceCustomIndex = blas.instanceCustomIndex;
    instance.mask = blas.mask;
    instance.instanceShaderBindingTableRecordOffset =
                                    blas.instanceShaderBindingTableRecordOffset;
    instance.flags = blas.flags;

    instance.accelerationStructureReference = blas.blas->deviceAddress();

    instances.push_back({instance});
  }
  return instances;
}

VkAccelerationStructureBuildSizesInfoKHR TLAS::_getCreateSizeInfo(
                  const VkAccelerationStructureGeometryKHR& geometryInfo) const
{
  VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
  buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometryInfo.geometryCount = 1;
  buildGeometryInfo.pGeometries = &geometryInfo;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  uint32_t blasCount = (uint32_t)_blases.size();
  VkResult result =
    _device.extFunctions().vkGetAccelerationStructureBuildSizesKHR(
                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                &buildGeometryInfo,
                                &blasCount,
                                &sizeInfo);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Unable to call vkGetAccelerationStructureBuildSizesKHR. Check the extension VK_KHR_acceleration_structure.");
  }
  return sizeInfo;
}

VkAccelerationStructureBuildSizesInfoKHR TLAS::_getUpdateSizeInfo(
                  const VkAccelerationStructureGeometryKHR& geometryInfo) const
{
  VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
  buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
  buildGeometryInfo.srcAccelerationStructure = _handle;
  buildGeometryInfo.dstAccelerationStructure = _handle;
  buildGeometryInfo.geometryCount = 1;
  buildGeometryInfo.pGeometries = &geometryInfo;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
  sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  uint32_t blasCount = (uint32_t)_blases.size();
  VkResult result =
    _device.extFunctions().vkGetAccelerationStructureBuildSizesKHR(
                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                &buildGeometryInfo,
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
  producer.buildTLAS(*this, *_geometryBuffer, *_scratchBuffer);
}

void TLAS::update(const BLASInstances& instances,
                  CommandProducerCompute& producer)
{
  MT_ASSERT(instances.size() == _blases.size())
  for(size_t i = 0; i < instances.size(); i++)
  {
    MT_ASSERT(_blases[i].blas == instances[i].blas);
    MT_ASSERT(_blases[i].instanceCustomIndex == instances[i].instanceCustomIndex);
    MT_ASSERT(_blases[i].instanceShaderBindingTableRecordOffset == instances[i].instanceShaderBindingTableRecordOffset);
    MT_ASSERT(_blases[i].flags == instances[i].flags);
    _blases[i].transform = instances[i].transform;
    _blases[i].mask = instances[i].mask;
  }
  producer.updateTLAS(*this, *_geometryBuffer, *_scratchBuffer);
}
