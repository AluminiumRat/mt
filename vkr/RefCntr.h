#pragma once

#include <atomic>

namespace mt
{
  // Счетчик ссылок. Работает вместе с RefCntrReference и шаблоном Ref
  // Позволяет реализовать систему умных указателей с подсчетом ссылок
  // внутри объекта
  class RefCntr
  {
  public:
    friend class RefCntrReference;

    inline RefCntr() noexcept;
    RefCntr(const RefCntr&) = delete;
    RefCntr& operator = (const RefCntr&) = delete;

  protected:
    // Уничтожить объект может только указатель. Деструкторы наследников
    // также необходимо помещать в protected секцию
    virtual ~RefCntr() noexcept = default;

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

  inline RefCntr::RefCntr() noexcept :
    _counter(0)
  {
  }

  inline void RefCntr::_incrementCounter() const noexcept
  {
    ++_counter;
  }

  inline int RefCntr::_decrementCounter() const noexcept
  {
    return --_counter;
  }
}