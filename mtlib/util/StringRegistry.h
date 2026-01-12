#pragma once

#include <unordered_map>
#include <mutex>
#include <string>

#include <util/SpinLock.h>

namespace mt
{
  //  Создает соответствие string <-> size_t
  //  По стуи просто обертка вокруг мапы
  class StringRegistry
  {
  public:
    StringRegistry();
    StringRegistry(const StringRegistry&) = delete;
    StringRegistry& operator = (const StringRegistry&) = delete;
    ~StringRegistry() noexcept = default;

    //  Индексы начинаются с 0 и увеличивааются на 1 для каждой новой строки
    //  Потокобезопасный метод
    size_t getIndex(const std::string& theString);

  private:
    SpinLock _accessMutex;
    size_t _indexCursor;
    using RegistryMap = std::unordered_map<std::string, size_t>;
    RegistryMap _map;
  };
}