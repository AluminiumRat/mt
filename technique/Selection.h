#pragma once

#include <optional>
#include <string>

#include <technique/TechniqueConfiguration.h>
#include <util/Assert.h>

namespace mt
{
  //  Дефайн в шейдере, который может принимать только ограниченное количество
  //    значений. Позволяет заранее скомпилировать все возможные вариации
  //    пайплайна и во время рендера просто выбирать нужный вариант.
  //  Публичный интерфейс для использования снаружи Technique
  class Selection
  {
  public:
    //  resvisionCounter - внешний счетчик ревизий. Он общий для всех
    //    селекшенов в технике, чтобы не проверять каждый раз все селекшены на
    //    апдэйт значений.
    Selection(const char* name,
              size_t& resvisionCounter,
              const TechniqueConfiguration* configuration);
    Selection(const Selection&) = delete;
    Selection& operator = (const Selection&) = delete;
    ~Selection() noexcept = default;

    inline const std::string& name() const noexcept;

    inline const std::string& value() const noexcept;
    void setValue(const std::string& newValue);

  protected:
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    size_t& _resvisionCounter;

    const TechniqueConfiguration::SelectionDefine* _description;
    uint32_t _valueIndex;
    uint32_t _valueWeight;

    //  Здесь хранятся имя и значение, если конфигурация не подключена,
    //  либо если в конфигурации нет селекшена с таким именем
    std::string _savedName;
    std::string _savedValue;
  };

  //  Дополнение для внутреннего использования внутри техники
  class SelectionImpl : public Selection
  {
  public:
    inline SelectionImpl( const char* name,
                          size_t& resvisionCounter,
                          const TechniqueConfiguration* configuration);
    SelectionImpl(const SelectionImpl&) = delete;
    SelectionImpl& operator = (const SelectionImpl&) = delete;
    ~SelectionImpl() noexcept = default;

    inline void setConfiguration(const TechniqueConfiguration* configuration);
    inline uint32_t valueWeight() const noexcept;
  };

  inline const std::string& Selection::name() const noexcept
  {
    if(_description != nullptr) return _description->name;
    else return _savedName;
  }

  inline const std::string& Selection::value() const noexcept
  {
    if(_description != nullptr) return _description->valueVariants[_valueIndex];
    else return _savedValue;
  }

  inline SelectionImpl::SelectionImpl(
                                  const char* name,
                                  size_t& resvisionCounter,
                                  const TechniqueConfiguration* configuration) :
    Selection(name, resvisionCounter, configuration)
  {
  }

  inline void SelectionImpl::setConfiguration(
                                    const TechniqueConfiguration* configuration)
  {
    _bindToConfiguration(configuration);
  }

  inline uint32_t SelectionImpl::valueWeight() const noexcept
  {
    return _valueWeight;
  }
}