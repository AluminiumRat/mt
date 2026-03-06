#pragma once

#include <vector>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class Device;

  class BLASAsset : public RefCounter
  {
  public:
    struct Geometry
    {
      //  Не должен быть nullptr
      ConstRef<DataBuffer> positions;
      //  Сожержит 32-х разрядные индексы
      //  Может быть nullptr
      ConstRef<DataBuffer> indices;
    };

  public:
    // geometryData не должен быть пустым
    BLASAsset(const std::vector<Geometry>& geometryData);
    BLASAsset(const BLASAsset&) = delete;
    BLASAsset& operator = (const BLASAsset&) = delete;
  protected:
    virtual ~BLASAsset() noexcept;
  public:

  private:
    void _clear() noexcept;

  private:
    Device& _device;

    //  Храним указатели, чтобы защитить буферы от удаления
    std::vector<Geometry> _buffers;
  };
}