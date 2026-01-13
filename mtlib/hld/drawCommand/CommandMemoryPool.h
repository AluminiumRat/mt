#pragma once

#include <memory>
#include <vector>

#include <hld/drawCommand/CommandMemoryHolder.h>

namespace mt
{
  //  Растущий пул памяти для объектов класса DrawCommand (и его потомков).
  //  Память из кучи выделяется фиксированными кусками по мере необходимости
  //  Память под объекты из пула выделяется в методе emplace
  //  Вернуть память в пул можно только всю сразу с помощью метода reset
  class CommandMemoryPool
  {
  public:
    //  chunkSize - размер кусков памяти, которыми память будет выделяться
    //    из общей кучи в пул
    explicit CommandMemoryPool(size_t chunkSize);
    CommandMemoryPool(const CommandMemoryPool&) = delete;
    CommandMemoryPool& operator = (const CommandMemoryPool&) = delete;
    ~CommandMemoryPool() noexcept = default;

    //  Выделить из пула память под объект CommandType и сконструировать
    //  на этой памяти объект
    template<typename CommandType, typename... Args>
    inline CommandPtr emplace(Args&... args);

    //  Вернуть всю выделенную паиять обратно в пул.
    //  ВНИМАНИЕ! Этот метод не вызывает деструкторы созданных объектов.
    //    Убедитесь, что все созданные с помощью пула команды уже не
    //    используются
    void reset() noexcept;

  private:
    void addHolder();
    void selectHolder(size_t holderIndex) noexcept;

  private:
    size_t _holderSize;
    using Holders = std::vector<std::unique_ptr<CommandMemoryHolder>>;
    Holders _holders;
    size_t _currentHolderIndex;
    CommandMemoryHolder* _currentHolder;
  };

  template<typename CommandType, typename... Args>
  inline CommandPtr CommandMemoryPool::emplace(Args&... args)
  {
    if(_holders.empty())
    {
      addHolder();
      selectHolder(0);
    }

    if(_currentHolder->memoryLeft() < sizeof(CommandType))
    {
      if(_currentHolderIndex == _holders.size() - 1) addHolder();
      selectHolder(_currentHolderIndex + 1);
    }

    return _currentHolder->emplace<CommandType>(args...);
  }
}