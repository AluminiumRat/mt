#pragma once

#include <atomic>

namespace mt
{
  // Счетчик ссылок. Работает вместе с RefCounterReference и шаблоном Ref
  // Позволяет реализовать систему умных указателей с подсчетом ссылок
  // внутри объекта
  class RefCounter
  {
  public:
    friend class RefCounterReference;

    inline RefCounter() noexcept;
    RefCounter(const RefCounter&) = delete;
    RefCounter& operator = (const RefCounter&) = delete;

    inline int counterValue() const noexcept;

  protected:
    // Уничтожить объект может только указатель. Деструкторы наследников
    // также необходимо помещать в protected секцию
    virtual ~RefCounter() noexcept = default;

  private:
    // Инкрементить и декрементить счетчики могут только указатели
    // const добавлен для того чтобы иметь возможность ссылаться на константный
    // объект. То есть счетчик ссылок не является внутренним состоянием
    // объекта.
    inline void _incrementCounter() const noexcept;
    inline int _decrementCounter() const noexcept;

  private:
    mutable std::atomic<int> _counter;
  };

  inline RefCounter::RefCounter() noexcept :
    _counter(0)
  {
  }

  inline int RefCounter::counterValue() const noexcept
  {
    return _counter;
  }

  inline void RefCounter::_incrementCounter() const noexcept
  {
    ++_counter;
  }

  inline int RefCounter::_decrementCounter() const noexcept
  {
    return --_counter;
  }
}