#include <util/Assert.h>
#include <vkr/queue/UniformMemoryPool.h>
#include <vkr/Device.h>
#include <vkr/PhysicalDevice.h>

using namespace mt;

UniformMemoryPool::UniformMemoryPool( size_t initialSize, Device& device) :
  _device(device),
  _granularity(device.physicalDevice().properties().
                                        limits.minUniformBufferOffsetAlignment),
  _bufferSize(initialSize),
  _currentBufferIndex(0),
  _currentBuffer(nullptr),
  _bufferCursor(0)
{
  _bufferSize += _granularity - (_bufferSize % _granularity);
}

void UniformMemoryPool::start()
{
  MT_ASSERT(!_mapper.has_value());
  if(_buffers.size() == 0) _addBuffer();
  _openBuffer(0);
}

void UniformMemoryPool::_addBuffer()
{
  Ref<PlainBuffer> newBuffer(new PlainBuffer(
                                        _device,
                                        _bufferSize,
                                        PlainBuffer::VOLATILE_UNIFORM_BUFFER));
  _buffers.push_back(std::move(newBuffer));
}

void UniformMemoryPool::_openBuffer(size_t bufferIndex)
{
  _currentBufferIndex = bufferIndex;
  _currentBuffer = _buffers[bufferIndex].get();
  _bufferCursor = 0;
  _mapper.emplace(*_currentBuffer, PlainBuffer::Mapper::CPU_TO_GPU);
}

void UniformMemoryPool::finish() noexcept
{
  MT_ASSERT(_mapper.has_value());
  _mapper.reset();
}

UniformMemoryPool::MemoryInfo UniformMemoryPool::write( const char* data,
                                                        size_t dataSize)
{
  size_t chunkSize = dataSize;
  if(dataSize % _granularity != 0)
  {
    chunkSize += _granularity - (dataSize % _granularity);
  }

  MT_ASSERT(_mapper.has_value());
  MT_ASSERT(chunkSize <= _bufferSize);

  if(_bufferCursor + chunkSize > _bufferSize)
  {
    if(_currentBufferIndex == _buffers.size() - 1) _addBuffer();
    _openBuffer(_currentBufferIndex + 1);
  }

  MemoryInfo writeInfo;
  writeInfo.buffer = _currentBuffer;
  writeInfo.offset = _bufferCursor;

  memcpy((char*)_mapper.value().data() + _bufferCursor, data, dataSize);
  _bufferCursor += chunkSize;

  return writeInfo;
}
