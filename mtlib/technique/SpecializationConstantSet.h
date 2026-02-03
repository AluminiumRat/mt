#pragma once

#include <span>
#include <string>
#include <vector>

namespace mt
{
  //  Вспомогательный класс для настройки констант специализации внутри техники
  //  Хранит в едином буфере некоторый набор значений, а так же мапу,
  //    объясняющую, где что лежит
  class SpecializationConstantSet
  {
  public:
    //  Выставить константу
    inline void addConstant(const char* name, uint32_t value);
    inline void addConstant(const char* name, float value);

    //  Получить значение для константы
    inline std::span<const std::byte> getConstantData(const char* name) const;

  private:
    void _addConstant(const char* name, const void* data, uint32_t dataSize);

  private:
    struct ConstantRecord
    {
      std::string name;
      uint32_t shift;
      uint32_t size;
    };

    std::vector<ConstantRecord> _constantMap;
    std::vector<std::byte> _dataContainer;
  };

  inline void SpecializationConstantSet::addConstant( const char* name,
                                                      uint32_t value)
  {
    _addConstant(name, &value, sizeof(value));
  }
  
  inline void SpecializationConstantSet::addConstant( const char* name,
                                                      float value)
  {
    _addConstant(name, &value, sizeof(value));
  }

  inline std::span<const std::byte>
              SpecializationConstantSet::getConstantData(const char* name) const
  {
    for(const ConstantRecord& record : _constantMap)
    {
      if(record.name == name)
      {
        return std::span(_dataContainer.data() + record.shift,
                         record.size);
      }
    }
    return {};
  }
}