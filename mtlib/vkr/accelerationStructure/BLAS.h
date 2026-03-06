#pragma once

#include <span>
#include <string>
#include <vector>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/accelerationStructure/BLASGeometry.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  //  Bottom level acceleration structure для рэй трэйсинга
  class BLAS : public RefCounter
  {
  public:
    //  geometry не должен быть пустым
    //  Внимание! Конструктор создает хэндл и сохраняет буферы ресурсов, но
    //    не создает саму структуру. Для того чтобы BLAS была пригодна к
    //    использованию, необходимо вызвать метод build
    BLAS( std::span<const BLASGeometry> geometry, const char* debugName);
    BLAS(const BLAS&) = delete;
    BLAS& operator = (const BLAS&) = delete;
  protected:
    virtual ~BLAS() noexcept;
  public:

    inline Device& device() const noexcept;
    inline VkAccelerationStructureKHR handle() const noexcept;

    inline const std::vector<BLASGeometry>& geometry() const noexcept;

    //  producer используется для отправки команд на создание BLAS-а. Т.е.
    //    фактически BLAS не будет готова на GPU до сабмита producer-а в очередь
    void build(CommandProducerCompute& producer);

  private:
    void _clear() noexcept;
    void _makeHandle();
    VkAccelerationStructureBuildSizesInfoKHR _getSizeInfo(
        const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const;

  private:
    Device& _device;
    std::string _debugName;

    ConstRef<DataBuffer> _blasBuffer;
    ConstRef<DataBuffer> _scratchBuffer;
    VkAccelerationStructureKHR _handle;

    //  Храним указатели на геометрию, чтобы защитить буферы от удаления
    std::vector<BLASGeometry> _geometry;
  };

  inline Device& BLAS::device() const noexcept
  {
    return _device;
  }
  
  inline VkAccelerationStructureKHR BLAS::handle() const noexcept
  {
    return _handle;
  }

  inline const std::vector<BLASGeometry>& BLAS::geometry() const noexcept
  {
    return _geometry;
  }
}