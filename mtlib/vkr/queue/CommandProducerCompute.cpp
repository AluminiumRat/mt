#include <util/Assert.h>
#include <vkr/accelerationStructure/BLAS.h>
#include <vkr/accelerationStructure/BLASGeometry.h>
#include <vkr/accelerationStructure/TLAS.h>
#include <vkr/pipeline/ComputePipeline.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/Device.h>

using namespace mt;

CommandProducerCompute::CommandProducerCompute( CommandPoolSet& poolSet,
                                                const char* debugName) :
  CommandProducerTransfer(poolSet, debugName)
{
}

void CommandProducerCompute::finalizeCommands() noexcept
{
  _computePipeline.reset();
  _computeDescriptors.clear();
  CommandProducerTransfer::finalizeCommands();
}

void CommandProducerCompute::restoreBindings(CommandBuffer& buffer)
{
  if(_computePipeline != nullptr)
  {
    vkCmdBindPipeline(buffer.handle(),
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      _computePipeline->handle());
  }

  for(uint32_t setIndex = 0;
      setIndex < _computeDescriptors.size();
      setIndex++)
  {
    const BindingRecord& binding = _computeDescriptors[setIndex];
    if(binding.descriptors == nullptr) continue;
    MT_ASSERT(binding.layout != nullptr);

    VkDescriptorSet setHandle = binding.descriptors->handle();
    vkCmdBindDescriptorSets(buffer.handle(),
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            binding.layout->handle(),
                            setIndex,
                            1,
                            &setHandle,
                            0,
                            nullptr);
  }

  CommandProducerTransfer::restoreBindings(buffer);
}

void CommandProducerCompute::bindComputePipeline(const ComputePipeline& pipeline)
{
  if(&pipeline == _computePipeline.get()) return;

  lockResource(pipeline);

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindPipeline(buffer.handle(),
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline.handle());

  _computePipeline = &pipeline;
}

void CommandProducerCompute::unbindComputePipeline()
{
  _computePipeline = nullptr;
}

void CommandProducerCompute::bindDescriptorSetCompute(
                                            const DescriptorSet& descriptorSet,
                                            uint32_t setIndex,
                                            const PipelineLayout& layout)
{
  MT_ASSERT(descriptorSet.isFinalized());

  if(setIndex >= _computeDescriptors.size())
  {
    _computeDescriptors.resize(setIndex + 1);
  }
  if(_computeDescriptors[setIndex].descriptors == &descriptorSet) return;

  lockResource(layout);
  lockResource(descriptorSet);
  addMultipleImagesUsage(descriptorSet.imagesAccess().accessTable());

  VkDescriptorSet setHandle = descriptorSet.handle();
  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdBindDescriptorSets(buffer.handle(),
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          layout.handle(),
                          setIndex,
                          1,
                          &setHandle,
                          0,
                          nullptr);

  _computeDescriptors[setIndex] = BindingRecord{.layout{&layout},
                                                .descriptors{&descriptorSet}};
}

void CommandProducerCompute::unbindDescriptorSetCompute(
                                                    uint32_t setIndex) noexcept
{
  if(setIndex >= _computeDescriptors.size()) return;
  _computeDescriptors[setIndex] = BindingRecord{};
}

void CommandProducerCompute::dispatch(uint32_t gridSizeX,
                                      uint32_t gridSizeY,
                                      uint32_t gridSizeZ)
{
  MT_ASSERT(!insideRenderPass());

  CommandBuffer& buffer = getOrCreateBuffer();
  vkCmdDispatch(buffer.handle(), gridSizeX, gridSizeY, gridSizeZ);
}

void CommandProducerCompute::buildBLAS( const BLAS& blas,
                                        const DataBuffer& scratchBuffer)
{
  MT_ASSERT(!insideRenderPass());

  lockResource(blas);
  lockResource(scratchBuffer);

  std::vector<VkAccelerationStructureGeometryKHR> geometryInfo =
                                          makeBLASGeometryInfo(blas.geometry());
  MT_ASSERT(!geometryInfo.empty());

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometyInfo{};
  buildGeometyInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometyInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildGeometyInfo.geometryCount = (uint32_t)geometryInfo.size();
  buildGeometyInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometyInfo.pGeometries = geometryInfo.data();
  buildGeometyInfo.dstAccelerationStructure = blas.handle();
  buildGeometyInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress();

  std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
  ranges.reserve(blas.geometry().size());
  for(const BLASGeometry& geometry : blas.geometry())
  {
    uint32_t primitivesCount = geometry.indices != nullptr ?
                                (uint32_t)geometry.indexCount / 3 :
                                (uint32_t)geometry.vertexCount / 3;
    ranges.push_back({.primitiveCount = primitivesCount,
                      .primitiveOffset = 0,
                      .firstVertex = 0,
                      .transformOffset = 0});
  }
  const VkAccelerationStructureBuildRangeInfoKHR* rangesPtr = ranges.data();

  CommandBuffer& buffer = getOrCreateBuffer();
  queue().device().extFunctions().vkCmdBuildAccelerationStructuresKHR(
                                                              buffer.handle(),
                                                              1,
                                                              &buildGeometyInfo,
                                                              &rangesPtr);
}

void CommandProducerCompute::buildTLAS( const TLAS& tlas,
                                        const DataBuffer& geometryBuffer,
                                        const DataBuffer& scratchBuffer)
{
  MT_ASSERT(!insideRenderPass());

  lockResource(tlas);
  lockResource(geometryBuffer);
  lockResource(scratchBuffer);

  std::vector<VkAccelerationStructureInstanceKHR> instances =
                                                      tlas.makeInstancesInfo();
  size_t instancesSize = instances.size() * sizeof(instances[0]);
  uploadToBuffer(geometryBuffer, 0, instancesSize, instances.data());

  VkAccelerationStructureGeometryDataKHR geometryData{};
  geometryData.instances = VkAccelerationStructureGeometryInstancesDataKHR{};
  geometryData.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  geometryData.instances.arrayOfPointers = VK_FALSE;
  geometryData.instances.data.deviceAddress = geometryBuffer.deviceAddress();

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry = geometryData;

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
  buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildGeometryInfo.geometryCount = 1;
  buildGeometryInfo.pGeometries = &geometry;
  buildGeometryInfo.dstAccelerationStructure = tlas.handle();
  buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress();

  VkAccelerationStructureBuildRangeInfoKHR buildRange{};
  buildRange.primitiveCount = (uint32_t)tlas.blases().size();
  const VkAccelerationStructureBuildRangeInfoKHR* rangePtr = &buildRange;

  CommandBuffer& buffer = getOrCreateBuffer();
  queue().device().extFunctions().vkCmdBuildAccelerationStructuresKHR(
                                                              buffer.handle(),
                                                              1,
                                                              &buildGeometryInfo,
                                                              &rangePtr);
}

void CommandProducerCompute::updateTLAS(const TLAS& tlas,
                                        const DataBuffer& geometryBuffer,
                                        const DataBuffer& scratchBuffer)
{
  MT_ASSERT(!insideRenderPass());

  lockResource(tlas);
  lockResource(geometryBuffer);
  lockResource(scratchBuffer);

  std::vector<VkAccelerationStructureInstanceKHR> instances =
                                                      tlas.makeInstancesInfo();
  size_t instancesSize = instances.size() * sizeof(instances[0]);
  uploadToBuffer(geometryBuffer, 0, instancesSize, instances.data());

  VkAccelerationStructureGeometryDataKHR geometryData{};
  geometryData.instances = VkAccelerationStructureGeometryInstancesDataKHR{};
  geometryData.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  geometryData.instances.arrayOfPointers = VK_FALSE;
  geometryData.instances.data.deviceAddress = geometryBuffer.deviceAddress();

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  geometry.geometry = geometryData;

  VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
  buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
  buildGeometryInfo.geometryCount = 1;
  buildGeometryInfo.pGeometries = &geometry;
  buildGeometryInfo.srcAccelerationStructure = tlas.handle();
  buildGeometryInfo.dstAccelerationStructure = tlas.handle();
  buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress();

  VkAccelerationStructureBuildRangeInfoKHR buildRange{};
  buildRange.primitiveCount = (uint32_t)tlas.blases().size();
  const VkAccelerationStructureBuildRangeInfoKHR* rangePtr = &buildRange;

  CommandBuffer& buffer = getOrCreateBuffer();
  queue().device().extFunctions().vkCmdBuildAccelerationStructuresKHR(
                                                              buffer.handle(),
                                                              1,
                                                              &buildGeometryInfo,
                                                              &rangePtr);
}
