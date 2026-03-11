#pragma once

#include <mutex>
#include <vector>

#include <util/SpinLock.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  //  Централизованное хранилище буферов, которые используются для аплоада
  //    данных на GPU.
  //  Основной смысл - переиспользовать буферы, не создавая новые каждый раз,
  //    когда надо что-то загрузить на GPU
  //  Этот пул - чать класса CommandQueue. Выделение и освобождение
  //    буферов происходит через класс CommandPool
  //  Класс потокобезопасный
  class UploadingBufferPool
  {
  public:
    UploadingBufferPool(Device& device);
    UploadingBufferPool(const UploadingBufferPool&) = delete;
    UploadingBufferPool& operator = (const UploadingBufferPool&) = delete;
    ~UploadingBufferPool() noexcept = default;

    //  Получить буфер из пула. Если подходящего буфера не нашлось, то
    //    то будет создан новый буфер
    //  Возвращенный буфер будет иметь Usage = UPLOADING_BUFFER и размер не
    //    меньше чем requiredSize(но может быть и больше)
    ConstRef<DataBuffer> get(size_t requiredSize);

    //  Вернуть буфер после использования обратно в пул.
    //  ВНИМАНИЕ! После использования, означает, что ни в одной очереди команд
    //  этот буфер не используется.
    inline void putBack(const DataBuffer& buffer);

  private:
    Device& _device;
    using Buffers = std::vector<ConstRef<DataBuffer>>;
    Buffers _freeBuffers;
    SpinLock _accessMutex;
  };

  inline void UploadingBufferPool::putBack(const DataBuffer& buffer)
  {
    std::lock_guard lock(_accessMutex);
    _freeBuffers.push_back(ConstRef(&buffer));
  }
}