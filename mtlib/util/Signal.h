#pragma once

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>

#include <util/Assert.h>
#include <util/Log.h>

namespace mt
{
  //  Слот - объект, находящийся на стороне обсервера(принимающего сигнал)
  //  Вызывает какой-то метод обсервера при срабатывании сигнала.
  //  Внимание! Сигналы не следят за слотами, а слоты не сообщают сигналам
  //    о своём уничтожении, необходимо вручную вызывать Signal::removeSlot
  //    для каждого Signal::addSlot, либо использовать шаблонный класс
  //    Connection
  template<typename... Args>
  class Slot
  {
  public:
    template<typename Observer>
    inline Slot(Observer& observer, void(Observer::* slot)(Args...));
    Slot(const Slot&) = delete;
    Slot& operator = (const Slot&) = delete;
    ~Slot() noexcept = default;

    inline void invoke (Args... args) noexcept;

  private:
    std::function<void(Args...)> _function;
  };

  //  Сигнал - объект, находящийся на стороне осервэйбла(генерирующего сигналы)
  //  Хранит в себе ссылки на слоты, подключенные к нему. При вызове метода
  //    emit сигнализирует слотам, а через них и обсерверам
  //  Внимание! Сигналы не следят за слотами, а слоты не сообщают сигналам
  //    о своём уничтожении, необходимо вручную вызывать Signal::removeSlot
  //    для каждого Signal::addSlot, либо использовать шаблонный класс
  //    Connection
  template<typename... Args>
  class Signal
  {
  public:
    Signal() noexcept = default;
    Signal(const Signal&) = delete;
    Signal&operator = (const Signal&) = delete;
    inline ~Signal() noexcept;

    inline void addSlot(Slot<Args...>& slot);
    inline void removeSlot(Slot<Args...>& slot) noexcept;

    // Раздать обсерверам информацию о том, что произошло какое-то событие
    inline void emit(Args... args) noexcept;

  private:
    using Slots = std::vector<Slot<Args...>*>;
    Slots _slots;
  };

  //  RAII обертка вокруг соединения сигнала и слота. Автоматически разрывает
  //  соединение в деструкторе
  template<typename... Args>
  class Connection
  {
  public:
    inline Connection() noexcept;
    inline Connection(Signal<Args...>& signal, Slot<Args...>& slot);
    Connection(const Connection&) = delete;
    inline Connection(Connection&& other) noexcept;
    Connection& operator = (const Connection&) = delete;
    inline Connection& operator = (Connection&& other) noexcept;
    inline ~Connection() noexcept;

    //  Разорвать соединение до деструктора
    inline void disconnect() noexcept;

    //  Забыть о том что сигнал и слот соединены.
    //  Позже будет необходимо вручную вызвать Signal::removeSlot
    inline void release() noexcept;

  private:
    Signal<Args...>* _signal;
    Slot<Args...>* _slot;
  };

  template<typename... Args>
  template<typename Observer>
  inline Slot<Args...>::Slot( Observer& observer,
                              void(Observer::* slot)(Args...))
  {
    _function = [&observer, slot](Args... args)
                {
                  (observer.*slot)(args...);
                };
  }

  template<typename... Args>
  inline void Slot<Args...>::invoke(Args... args) noexcept
  {
    try
    {
      _function(args...);
    }
    catch(std::exception& error)
    {
      Log::error() << error.what();
    }
  }

  template<typename... Args>
  inline Signal<Args...>::~Signal() noexcept
  {
    MT_ASSERT(_slots.empty());
  }

  template<typename... Args>
  inline void Signal<Args...>::addSlot(Slot<Args...>& slot)
  {
    MT_ASSERT(std::find(_slots.begin(), _slots.end(), &slot) == _slots.end());
    _slots.push_back(&slot);
  }

  template<typename... Args>
  inline void Signal<Args...>::removeSlot(Slot<Args...>& slot) noexcept
  {
    typename Slots::iterator iSlot = std::find( _slots.begin(),
                                                _slots.end(),
                                                &slot);
    if(iSlot != _slots.end()) _slots.erase(iSlot);
  }

  template<typename... Args>
  inline void Signal<Args...>::emit(Args... args) noexcept
  {
    for(Slot<Args...>* slot : _slots)
    {
      slot->invoke(args...);
    }
  }

  template<typename... Args>
  inline Connection<Args...>::Connection() noexcept :
    _signal(nullptr),
    _slot(nullptr)
  {
  }

  template<typename... Args>
  inline Connection<Args...>::Connection( Signal<Args...>& signal,
                                          Slot<Args...>& slot) :
    _signal(nullptr),
    _slot(nullptr)
  {
    signal.addSlot(slot);
    _signal = &signal;
    _slot = &slot;
  }

  template<typename... Args>
  inline Connection<Args...>::Connection(Connection<Args...>&& other) noexcept :
    _signal(other._signal),
    _slot(other._slot)
  {
    other._signal = nullptr;
    other._slot = nullptr;
  }

  template<typename... Args>
  inline Connection<Args...>& Connection<Args...>::operator = (
                                          Connection<Args...>&& other) noexcept
  {
    disconnect();
    std::swap(_signal, other._signal);
    std::swap(_slot, other._slot);
    return *this;
  }

  template<typename... Args>
  inline Connection<Args...>::~Connection() noexcept
  {
    disconnect();
  }

  template<typename... Args>
  inline void Connection<Args...>::disconnect() noexcept
  {
    if(_signal != nullptr)
    {
      MT_ASSERT(_slot != nullptr);
      _signal->removeSlot(*_slot);
    }
  }

  template<typename... Args>
  inline void Connection<Args...>::release() noexcept
  {
    _signal = nullptr;
    _slot = nullptr;
  }
}