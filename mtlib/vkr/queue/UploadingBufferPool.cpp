#include <vkr/queue/UploadingBufferPool.h>

#define UPLOADING_BUFFER_MINIMAL_SIZE (size_t)(1024 * 1024 * 1024)

using namespace mt;

UploadingBufferPool::UploadingBufferPool(Device& device) :
  _device(device)
{
}

ConstRef<DataBuffer> UploadingBufferPool::get(size_t requiredSize)
{
  {
    //  Для начала ищем в ранее уже созданных буферах
    std::lock_guard lock(_accessMutex);
    for(int iBuffer = (int)_freeBuffers.size() - 1; iBuffer > 0; iBuffer--)
    {
      if(_freeBuffers[iBuffer]->size() >= requiredSize)
      {
        ConstRef<DataBuffer> buffer = _freeBuffers[iBuffer];
        _freeBuffers.erase(_freeBuffers.begin() + iBuffer);
        return buffer;
      }
    }
  }
  //  Ничего не нашли. Создаем новый буфер
  return ConstRef(new DataBuffer( _device,
                                  std::max( requiredSize,
                                            UPLOADING_BUFFER_MINIMAL_SIZE),
                                  DataBuffer::UPLOADING_BUFFER,
                                  "Common uploading buffer"));
}