#pragma once

#include <glm/glm.hpp>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/accelerationStructure/BLAS.h>
#include <vkr/accelerationStructure/BLASInstance.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Device;

  //  Top level acceleration structure для рэй трэйсинга
  class TLAS : public RefCounter
  {
  public:
    TLAS( Device& device,
          const BLASInstances& instances,
          const char* debugName);
    TLAS(const TLAS&) = delete;
    TLAS& operator = (const TLAS&) = delete;
  protected:
    virtual ~TLAS() noexcept;
  public:

    inline Device& device() const noexcept;
    inline VkAccelerationStructureKHR handle() const noexcept;
    inline const std::string& debugName() const noexcept;

    inline const BLASInstances& blases() const noexcept;
    std::vector<VkAccelerationStructureInstanceKHR> makeInstancesInfo() const;

    void build(CommandProducerCompute& producer);

  private:
    void _clear() noexcept;
    void _makeHandle();
    VkAccelerationStructureBuildSizesInfoKHR _getSizeInfo(
        const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const;

  private:
    Device& _device;

    ConstRef<DataBuffer> _tlasBuffer;
    ConstRef<DataBuffer> _scratchBuffer;
    ConstRef<DataBuffer> _geometryBuffer;
    VkAccelerationStructureKHR _handle;

    std::string _debugName;

    BLASInstances _blases;
  };

  inline Device& TLAS::device() const noexcept
  {
    return _device;
  }

  inline VkAccelerationStructureKHR TLAS::handle() const noexcept
  {
    return _handle;
  }

  inline const std::string& TLAS::debugName() const noexcept
  {
    return _debugName;
  }

  inline const BLASInstances& TLAS::blases() const noexcept
  {
    return _blases;
  }
}