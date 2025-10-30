#include <util/Assert.h>
#include <vkr/queue/UniformMemoryPool.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>

using namespace mt;

UniformMemoryPool::Session::Session(UniformMemoryPool& pool) :
  _pool(&pool),
  _currentBufferIndex(0),
  _currentBuffer(nullptr),
  _bufferCursor(0)
{
  MT_ASSERT(!_pool->_isSessionOpened);

  if (_pool->_buffers.empty()) _pool->_addBuffer();
  _openBuffer(0);

  _pool->_isSessionOpened = true;
}

void UniformMemoryPool::Session::_openBuffer(size_t bufferIndex)
{
  _currentBufferIndex = bufferIndex;
  _currentBuffer = _pool->_buffers[bufferIndex].get();
  _bufferCursor = 0;
  _mapper.emplace(*_currentBuffer, DataBuffer::Mapper::CPU_TO_GPU);
}

UniformMemoryPool::MemoryInfo UniformMemoryPool::Session::write(
                                                              const char* data,
                                                              size_t dataSize)
{
  MT_ASSERT(_pool != nullptr);
  MT_ASSERT(_mapper.has_value());

  // Определяем, сколько памяти в буфере нам надо, чтобы сохранить данные
  size_t chunkSize = dataSize;
  if(dataSize % _pool->_granularity != 0)
  {
    chunkSize += _pool->_granularity - (dataSize % _pool->_granularity);
  }
  MT_ASSERT(chunkSize <= _pool->_bufferSize);

  // Проверка, хватит ли места в текущем буфере
  if(_bufferCursor + chunkSize > _pool->_bufferSize)
  {
    // Если места не хватает и в пуле закончились буферы, то создаем новый буфер
    if(_currentBufferIndex == _pool->_buffers.size() - 1)
    {
      _pool->_addBuffer();
    }
    _openBuffer(_currentBufferIndex + 1);
  }

  MemoryInfo writeInfo;
  writeInfo.buffer = _currentBuffer;
  writeInfo.offset = _bufferCursor;

  memcpy((char*)_mapper.value().data() + _bufferCursor, data, dataSize);
  _bufferCursor += chunkSize;

  return writeInfo;
}

void UniformMemoryPool::Session::finish() noexcept
{
  if(_pool == nullptr) return;
  _mapper.reset();
  _pool->_isSessionOpened = false;
  _pool = nullptr;
}

UniformMemoryPool::UniformMemoryPool( size_t initialSize, Device& device) :
  _device(device),
  _granularity(device.physicalDevice().properties().
                                        limits.minUniformBufferOffsetAlignment),
  _bufferSize(initialSize),
  _isSessionOpened(false)
{
  // Размер буфера должен быть кратен limits.minUniformBufferOffsetAlignment
  if(_bufferSize % _granularity != 0)
  {
    _bufferSize += _granularity - (_bufferSize % _granularity);
  }
}

void UniformMemoryPool::_addBuffer()
{
  Ref<DataBuffer> newBuffer(new DataBuffer(
                                        _device,
                                        _bufferSize,
                                        DataBuffer::VOLATILE_UNIFORM_BUFFER));
  _buffers.push_back(std::move(newBuffer));
}
