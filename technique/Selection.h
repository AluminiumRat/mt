#pragma once

#include <string>

#include <technique/TechniqueConfiguration.h>
#include <util/Assert.h>

namespace mt
{
  class Technique;
  struct TechniqueVolatileContext;

  //  Компонент класса Technique
  //  Дефайн в шейдере, который может принимать только ограниченное количество
  //    значений. Позволяет заранее скомпилировать все возможные вариации
  //    пайплайна и во время рендера просто выбирать нужный вариант.
  //  Публичный интерфейс для использования снаружи Technique
  class Selection
  {
  public:
    //  selectionIndex - индекс селекшена в списке техники, нужен для
    //    волатильного контекста.
    Selection(const char* name,
              const Technique& technique,
              const TechniqueConfiguration* configuration,
              uint32_t selectionIndex);
    Selection(const Selection&) = delete;
    Selection& operator = (const Selection&) = delete;
    ~Selection() noexcept = default;

    inline const std::string& name() const noexcept;

    inline const std::string& value() const noexcept;
    void setValue(const std::string& newValue);

    //  Выставить значение в волатильный контекст, а не в сам селекшен.
    //  Позволяет корректировать выбор пайплайна непосредственно во время
    //    рендера, не внося изменений в технику.
    void setValue(const std::string& newValue,
                  TechniqueVolatileContext& context) const;

  protected:
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    const Technique& _technique;

    const TechniqueConfiguration::SelectionDefine* _description;

    uint32_t _valueIndex;   //  Номер выбранного значения в списке возможных
                            //  значений
    uint32_t _valueWeight;  //  Вес выбранного значения при выборе варианта
                            //  пайплайна

    //  Индекс селекшена в списке техники. По нему выставляется значение внутри
    //    волатильного контекста.
    uint32_t _selectionIndex;

    //  Здесь хранятся имя и значение, если конфигурация не подключена,
    //  либо если в конфигурации нет селекшена с таким именем
    std::string _savedName;
    std::string _savedValue;
  };

  //  Дополнение для внутреннего использования внутри техники
  class SelectionImpl : public Selection
  {
  public:
    //  selectionIndex - индекс селекшена в списке техники
    inline SelectionImpl( const char* name,
                          const Technique& technique,
                          const TechniqueConfiguration* configuration,
                          uint32_t selectionIndex);
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
                                  const Technique& technique,
                                  const TechniqueConfiguration* configuration,
                                  uint32_t selectionIndex) :
    Selection(name, technique, configuration, selectionIndex)
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