#pragma once

#include <optional>
#include <vector>

#include <vkr/PlainBuffer.h>
#include <vkr/Ref.h>

namespace mt
{
  //  Набор переиспользуемых юниформ-буферов с часто меняемыми данными
  //    (PlainBuffer::VOLATILE_UNIFORM_BUFFER)
  //  Используется во время заполнения буферов комманд для передачи юниформ
  //    переменных.
  //  Предполагает использование сессиями. Сессия начинается с вызова метода
  //    UniformMemoryPool::start. Во время сессии пул наполняется данными,
  //    переиспользуя старые буферы или создавая новые по мере необходимости.
  //    При этом расход памяти постоянно растет, записанные данные не
  //    уничтожаются. Сессия заканчивается вызовом UniformMemoryPool::finish,
  //    после чего записанные буферы можно использовать в очереди команд.
  //    Стартовать новую сессию можно не рашьше, чем очередь команд закончит
  //    использовать записанные данные. После старта записанные данные будут
  //    потеряны, пул начнет наполняться по новой.
  class UniformMemoryPool
  {
  public:
    // Информация о том, где можно забрать записанные данные на стороне GPU.
    struct MemoryInfo
    {
      PlainBuffer* buffer;
      size_t offset;
    };

  public:
    UniformMemoryPool(size_t initialSize, Device& device);
    UniformMemoryPool(const UniformMemoryPool&) = delete;
    UniformMemoryPool& operator = (const UniformMemoryPool&) = delete;
    ~UniformMemoryPool() = default;

    //  Старт новой сессии работы с пулом.
    //  ВНИМАНИЕ! Данные, записанные в пул в предыдущей сессии будут потеряны.
    //    Очереди команд не должны использовать эти данные к моменту старта
    //    сессии.
    void start();

    inline bool isStarted() const noexcept;

    //  Закончить сессию. После этого вызова данные, записанные в пул будут
    //  доступны для чтения со стороны GPU.
    void finish() noexcept;

    template <typename DataType>
    inline MemoryInfo write(const DataType& data);
    template <typename DataType>
    inline MemoryInfo write(const std::vector<DataType>& data);

    MemoryInfo write(const char* data, size_t size);

  private:
    void _addBuffer();
    void _openBuffer(size_t bufferIndex);

  private:
    Device& _device;
    size_t _granularity;
    size_t _bufferSize;
  
    using Buffers = std::vector<Ref<PlainBuffer>>;
    Buffers _buffers;
    size_t _currentBufferIndex;
    PlainBuffer* _currentBuffer;
    size_t _bufferCursor;

    std::optional<PlainBuffer::Mapper> _mapper;
  };

  inline bool UniformMemoryPool::isStarted() const noexcept
  {
    return _mapper.has_value();
  }

  template <typename DataType>
  inline UniformMemoryPool::MemoryInfo
                    UniformMemoryPool::write(const std::vector<DataType>& data)
  {
    return write((const char*)(data.data()), sizeof(DataType) * data.size());
  }

  template <typename DataType>
  inline UniformMemoryPool::MemoryInfo UniformMemoryPool::write(
                                                          const DataType& data)
  {
    return write((const char*)(&data), sizeof(DataType));
  }
}