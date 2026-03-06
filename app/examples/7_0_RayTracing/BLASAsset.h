#pragma once

#include <string>
#include <vector>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;

  class BLASAsset : public RefCounter
  {
  public:
    struct Geometry
    {
      //  Содержит положения вершин. По 3 float-а на вершину буз разрывов
      //  Не должен быть nullptr
      ConstRef<DataBuffer> positions;
      //  Количество вершин в positions. Не должен быть 0
      size_t vertexCount;
      //  Сожержит 32-х разрядные индексы
      //  Может быть nullptr
      ConstRef<DataBuffer> indices;
      //  Количество индексов в indices. Если indices не nullptr, то не может
      //  быть 0. Если indeces == nullptr, то значение indexCount игнорируется
      size_t indexCount;
    };

  public:
    //  geometryData не должен быть пустым
    //  producer используется для отправки команд на создание BLAS-а. Т.е.
    //    фактически BLAS не будет готова на GPU до сабмита producer-а в очередь
    BLASAsset(const std::vector<Geometry>& geometryData,
              CommandProducerGraphic& producer,
              const char* debugName);
    BLASAsset(const BLASAsset&) = delete;
    BLASAsset& operator = (const BLASAsset&) = delete;
  protected:
    virtual ~BLASAsset() noexcept;
  public:

    inline Device& device() const noexcept;
    inline VkAccelerationStructureKHR handle() const noexcept;

  private:
    void _clear() noexcept;
    void _makeHandle(CommandProducerGraphic& producer);
    std::vector<VkAccelerationStructureGeometryKHR> _makeGeometryInfo() const;
    VkAccelerationStructureBuildSizesInfoKHR _getSizeInfo(
        const VkAccelerationStructureBuildGeometryInfoKHR& geometryInfo) const;

  private:
    Device& _device;
    std::string _debugName;

    ConstRef<DataBuffer> _blasBuffer;
    VkAccelerationStructureKHR _handle;

    //  Храним указатели на геометрию, чтобы защитить буферы от удаления
    std::vector<Geometry> _geometry;
  };

  inline Device& BLASAsset::device() const noexcept
  {
    return _device;
  }
  
  inline VkAccelerationStructureKHR BLASAsset::handle() const noexcept
  {
    return _handle;
  }
}