#include <technique/Selection.h>
#include <technique/Technique.h>
#include <util/Log.h>

using namespace mt;

#define DEFAULT_VALUE "__default"

Selection::Selection( const char* name,
                      const Technique& technique,
                      size_t& resvisionCounter,
                      const TechniqueConfiguration* configuration) :
  _technique(technique),
  _revisionCounter(resvisionCounter),
  _description(nullptr),
  _valueIndex(0),
  _valueWeight(0),
  _savedName(name),
  _savedValue(DEFAULT_VALUE)
{
  _bindToConfiguration(configuration);
}

inline void Selection::setValue(const std::string& newValue)
{
  if(_description != nullptr)
  {
    //  У нас есть нормальная привязка к конфигурации
    //  Для начала ищем вариант, который нам передали
    uint32_t newVariantIndex = 0;
    if(newValue != DEFAULT_VALUE)
    {
      for(;
          newVariantIndex < _description->valueVariants.size();
          newVariantIndex++)
      {
        if(_description->valueVariants[newVariantIndex] == newValue) break;
      }
    }
    if(newVariantIndex == _description->valueVariants.size())
    {
      //  Такого варианта нет, используем нулевой
      Log::warning() << _technique.debugName() << " : selection " << name() << " doesn't have variant: " << newValue;
      newVariantIndex = 0;
    }

    // Выставляем рабочие значения для этого варианта
    if (_valueIndex != newVariantIndex)
    {
      _valueIndex = newVariantIndex;
      _valueWeight = _valueIndex * _description->weight;
      _revisionCounter++;
    }
  }
  else
  {
    //  Привязки к конфигурации нет, просто сохраняем значение на случай, если
    //  конфигурация появится
    _savedValue = newValue;
  }
}

void Selection::_bindToConfiguration(
                                    const TechniqueConfiguration* configuration)
{
  if(configuration == nullptr && _description == nullptr) return;

  const std::string& myName = name();
  const std::string& myValue = value();

  if(configuration != nullptr)
  {
    // Ищем селекшен в новой конфигурации и пытаемся привязаться к нему
    for(const TechniqueConfiguration::SelectionDefine& description :
                                                      configuration->selections)
    {
      if(description.name == myName)
      {
        _description = &description;
        setValue(myValue);
        _revisionCounter++;
        _savedName = std::string();
        _savedValue = std::string();
        return;
      }
    }
  }

  // Либо новая конфигурация nullptr, либо мы не нашли в ней селекшен
  _description = nullptr;
  _valueIndex = 0;
  _valueWeight = 0;
  _savedName = myName;
  _savedValue = myValue;
}
