#pragma once

#include <memory>

#include <hld/drawCommand/DrawCommand.h>
#include <util/Assert.h>

namespace mt
{
  //  Кастомный деструктор для умного указателя CommandPtr
  //  Уничтожает объект класса DrawCommand, но не освобождает память
  class CommandDestructor
  {
  public:
    CommandDestructor() noexcept = default;
    CommandDestructor(const CommandDestructor&) = default;
    CommandDestructor& operator = (const CommandDestructor&) = default;
    ~CommandDestructor() noexcept = default;

    inline void operator() (DrawCommand* command) const noexcept;
  };

  //  Кастомный unique_ptr на DrawCommand
  //  При удалении вызывает деструктор DrawCommand, но не освобождает память,
  //    так как управление памятью происходит централизованно через
  //    CommandMemoryHolder
  class CommandPtr : public std::unique_ptr<DrawCommand, CommandDestructor>
  {
  public:
    CommandPtr() noexcept = default;
    explicit inline CommandPtr(DrawCommand* p) noexcept;
    CommandPtr(const CommandPtr&) = delete;
    inline CommandPtr(CommandPtr&& other) noexcept;
    CommandPtr& operator = (const CommandPtr&) = delete;
    inline CommandPtr&  operator = (CommandPtr&& other) noexcept;
  };

  //  Пул памяти фиксированной величины для объектов класса DrawCommand (и
  //    его потомков).
  //  Память из кучи выделяется один раз в конструкторе.
  //  Память под объекты из пула выделяется в методе emplace
  //  Вернуть память в пул можно только всю сразу с помощью метода reset
  class CommandMemoryHolder
  {
  public:
    inline explicit CommandMemoryHolder(size_t size);
    CommandMemoryHolder(const CommandMemoryHolder&) = delete;
    CommandMemoryHolder& operator = (const CommandMemoryHolder&) = delete;
    ~CommandMemoryHolder() noexcept = default;

    //  Общий объем пула
    inline size_t size() const noexcept;
    //  Сколько памяти уже выделенно в пуле
    inline size_t allocated() const noexcept;
    //  Сколько памяти осталось в пуле
    inline size_t memoryLeft() const noexcept;

    //  Выделить из пула память под объект CommandType и сконструировать
    //  на этой памяти объект
    template<typename CommandType, typename... Args>
    inline CommandPtr emplace(Args&... args);

    //  Вернуть всю выделенную паиять обратно в пул.
    //  ВНИМАНИЕ! Этот метод не вызывает деструкторы созданных объектов.
    //    Убедитесь, что все созданные с помощью пула команды уже не
    //    используются
    inline void reset() noexcept;

  private:
    size_t _size;
    std::unique_ptr<char[]> _memory;
    size_t _allocated;
  };

  void CommandDestructor::operator()(DrawCommand* resource) const noexcept
  {
    resource->~DrawCommand();
  }

  CommandPtr::CommandPtr(DrawCommand* p) noexcept:
    std::unique_ptr<DrawCommand, CommandDestructor>(p)
  {
  }

  inline CommandPtr::CommandPtr(CommandPtr&& other) noexcept :
    std::unique_ptr<DrawCommand, CommandDestructor>(std::move(other))
  {
  }

  inline CommandPtr& CommandPtr::operator = (CommandPtr&& other) noexcept
  {
    std::unique_ptr<DrawCommand, CommandDestructor>::operator=(
                                                              std::move(other));
    return *this;
  }

  inline CommandMemoryHolder::CommandMemoryHolder(size_t size) :
    _size(size),
    _memory(new char[size]),
    _allocated(0)
  {
  }

  inline size_t CommandMemoryHolder::size() const noexcept
  {
    return _size;
  }

  inline size_t CommandMemoryHolder::allocated() const noexcept
  {
    return _allocated;
  }

  inline size_t CommandMemoryHolder::memoryLeft() const noexcept
  {
    return _size - _allocated;
  }

  template<typename CommandType, typename... Args>
  inline CommandPtr CommandMemoryHolder::emplace(Args&... args)
  {

    MT_ASSERT(sizeof(CommandType) <= memoryLeft());
    void* placePosition = _memory.get() + _allocated;

    CommandType* newCommand = new (placePosition) CommandType(args...);
    CommandPtr result(newCommand);
  
    _allocated += sizeof(CommandType);

    return result;
  }

  inline void CommandMemoryHolder::reset() noexcept
  {
    _allocated = 0;
  }
}