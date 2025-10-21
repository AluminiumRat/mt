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
  //  Предполагает использование сессиями. Сессия начинается с создания класса
  //    UniformMemoryPool::Session. Во время сессии пул наполняется данными,
  //    переиспользуя старые буферы или создавая новые по мере необходимости.
  //    При этом расход памяти постоянно растет, записанные данные не
  //    уничтожаются. Сессия заканчивается вызовом Session::finish, либо
  //    деструктором класса Session, после чего записанные буферы можно
  //    использовать в очереди команд.
  //  Стартовать новую сессию можно не рашьше, чем очередь команд закончит
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

    //  Класс, представляющий одну сессию работы с пулом. Сессия стартует
    //    прямо в конструкторе этого класса.
    //  ВНИМАНИЕ! Одновременно можно использовать только одну незакрытую сессию
    //    для одного пула.
    //  ВНИМАНИЕ! Данные, записанные в пул в предыдущей сессии будут потеряны.
    //    Очереди команд не должны использовать эти данные к моменту старта
    //    сессии.
    class Session
    {
    public:
      explicit Session(UniformMemoryPool& pool);
      Session(const Session&) = delete;
      Session& operator = (const Session&) = delete;
      inline Session(Session&& other) noexcept;
      inline Session& operator = (Session&& other) noexcept;
      inline ~Session() noexcept;

      template <typename DataType>
      inline MemoryInfo write(const DataType& data);
      template <typename DataType>
      inline MemoryInfo write(const std::vector<DataType>& data);

      MemoryInfo write(const char* data, size_t size);

      //  Закончить сессию. После этого вызова данные, записанные в пул будут
      //  доступны для чтения со стороны GPU.
      void finish() noexcept;

    private:
      void _openBuffer(size_t bufferIndex);

    private:
      UniformMemoryPool* _pool;

      size_t _currentBufferIndex;
      PlainBuffer* _currentBuffer;
      size_t _bufferCursor;

      std::optional<PlainBuffer::Mapper> _mapper;
    };

  public:
    UniformMemoryPool(size_t initialSize, Device& device);
    UniformMemoryPool(const UniformMemoryPool&) = delete;
    UniformMemoryPool& operator = (const UniformMemoryPool&) = delete;
    ~UniformMemoryPool() noexcept = default;

  private:
    void _addBuffer();

  private:
    Device& _device;
    size_t _granularity;
    size_t _bufferSize;
  
    using Buffers = std::vector<Ref<PlainBuffer>>;
    Buffers _buffers;

    bool _isSessionOpened;
  };

  inline UniformMemoryPool::Session::Session(Session&& other) noexcept :
    _pool(other._pool),
    _currentBufferIndex(other._currentBufferIndex),
    _currentBuffer(other._currentBuffer),
    _bufferCursor(other._bufferCursor),
    _mapper(std::move(other._mapper))
  {
    other._pool = nullptr;
    other._currentBufferIndex = 0;
    other._currentBuffer = nullptr;
    other._bufferCursor = 0;
  }

  inline UniformMemoryPool::Session& UniformMemoryPool::Session::operator =
                                  (UniformMemoryPool::Session&& other) noexcept
  {
    finish();

    _pool = other._pool;
    _currentBufferIndex = other._currentBufferIndex;
    _currentBuffer = other._currentBuffer;
    _bufferCursor = other._bufferCursor;
    _mapper = std::move(other._mapper);

    other._pool = nullptr;
    other._currentBufferIndex = 0;
    other._currentBuffer = nullptr;
    other._bufferCursor = 0;
  }

  inline UniformMemoryPool::Session::~Session() noexcept
  {
    finish();
  }

  template <typename DataType>
  inline UniformMemoryPool::MemoryInfo
            UniformMemoryPool::Session::write(const std::vector<DataType>& data)
  {
    return write((const char*)(data.data()), sizeof(DataType) * data.size());
  }

  template <typename DataType>
  inline UniformMemoryPool::MemoryInfo
                        UniformMemoryPool::Session::write(const DataType& data)
  {
    return write((const char*)(&data), sizeof(DataType));
  }
}