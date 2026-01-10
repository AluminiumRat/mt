#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace mt
{
  //  Мапа, которая умеет конвертировать значение enum-а в строку и обратно
  //  По факту, две хэш мапы, объединенные в один класс, для того чтобы можно
  //    было использовать один конструктор
  template <typename EnumType>
  class Bimap
  {
  public:
    using const_iterator =
                      std::unordered_map<std::string, EnumType>::const_iterator;

  public:
    inline Bimap(
          const char* debugName,
          std::initializer_list<std::pair<EnumType, const char*>> init);
    Bimap() = default;
    Bimap(const Bimap&) = default;
    Bimap(Bimap&&) noexcept = default;
    Bimap& operator = (const Bimap&) = default;
    Bimap& operator = (Bimap&&) noexcept = default;
    ~Bimap() noexcept = default;

    inline EnumType operator [](const std::string& name) const;
    inline const std::string& operator [](EnumType value) const;

    inline size_t size() const noexcept;
    inline const_iterator begin() const noexcept;
    inline const_iterator end() const noexcept;

  private:
    std::unordered_map<EnumType, std::string> _toString;
    std::unordered_map<std::string, EnumType> _toEnum;

    std::string _debugName;
  };

  template <typename EnumType>
  inline Bimap<EnumType>::Bimap(
        const char* debugName,
        std::initializer_list<std::pair<EnumType, const char*>> init) :
    _debugName(debugName)
  {
    _toString.reserve(init.size());
    _toEnum.reserve(init.size());
    for(std::pair<EnumType, const char*> valuesPair : init)
    {
      _toString[valuesPair.first] = valuesPair.second;
      _toEnum[valuesPair.second] = valuesPair.first;
    }
  }

  template <typename EnumType>
  inline EnumType Bimap<EnumType>::operator[](const std::string& name) const
  {
    auto i = _toEnum.find(name);
    if(i == _toEnum.end()) throw std::runtime_error(_debugName + ": unknown name: " + name);
    return i->second;
  }

  template <typename EnumType>
  inline const std::string& Bimap<EnumType>::operator[](EnumType value) const
  {
    auto i = _toString.find(value);
    if (i == _toString.end()) throw std::runtime_error(_debugName + ": unknown value: " + std::to_string((int)value));
    return i->second;
  }

  template <typename EnumType>
  inline size_t Bimap<EnumType>::size() const noexcept
  {
    return _toEnum.size();
  }

  template <typename EnumType>
  inline Bimap<EnumType>::const_iterator Bimap<EnumType>::begin() const noexcept
  {
    return _toEnum.begin();
  }

  template <typename EnumType>
  inline Bimap<EnumType>::const_iterator Bimap<EnumType>::end() const noexcept
  {
    return _toEnum.end();
  }
}