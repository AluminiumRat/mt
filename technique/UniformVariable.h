#pragma once

#include <memory>
#include <string>
#include <vector>

#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueUniformBlock.h>
#include <util/Assert.h>

namespace mt
{
  class Technique;
  class TechniqueVolatileContext;

  //  Компонент класса Technique
  //  Единичная именованная запись в юниформ буфере. Это могут быть
  //    отдельные переменные, компоненты структур или массивы
  //  Публичный интерфейс для использования снаружи Technique
  class UniformVariable
  {
  public:
    // Указатель на место в памяти, где хранится значение переменной
    struct ValueRef
    {
      const void* data;
      size_t dataSize;
    };

  public:
    UniformVariable(
            const char* name,
            const Technique& technique,
            const TechniqueConfiguration* configuration,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks);
    UniformVariable(const UniformVariable&) = delete;
    UniformVariable& operator = (const UniformVariable&) = delete;
    ~UniformVariable() noexcept = default;

    inline const std::string& name() const noexcept;

    //  Установить значение переменной, сохранив его в технике
    //  Значение сохраняется между циклами рендера
    template <typename DataType>
    inline void setValue(const DataType& data);
    template <typename DataType>
    inline void setValue(const std::vector<DataType>& data);
    void setValue(ValueRef newValue);
    //  Установить значение переменной в контекст отрисовки. Значение в технике
    //  не сохраняется и будет утеряно вместе с TechniqueVolatileContext.
    template <typename DataType>
    inline void setValue( TechniqueVolatileContext& context,
                          const DataType& data) const;
    template <typename DataType>
    inline void setValue( TechniqueVolatileContext& context, 
                          const std::vector<DataType>& data) const;
    void setValue(TechniqueVolatileContext& context, 
                  ValueRef newValue) const;

    //  Может вернуть data == nullptr, если значение ещё не было установлено
    inline ValueRef getValue() const noexcept;

    //  Битсет из элементов TechniqueConfiguration::GUIHints
    inline uint32_t guiHints() const noexcept;

  protected:
    void _bindToConfiguration(
            const TechniqueConfiguration* configuration,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks);
    struct UniformDescriptions
    {
      const TechniqueConfiguration::UniformBuffer* bufferDescription;
      const TechniqueConfiguration::UniformVariable* variableDescription;
    };
    //  Найти в конфигурации описание переменной и буфера, в котором она
    //  находится
    UniformDescriptions _findDescriptions(
                            const TechniqueConfiguration* configuration) const;
    //  Найти место, куда будем сохранять значения переменной
    TechniqueUniformBlock* _findStorage(
          UniformDescriptions descriptions,
          const std::vector<std::unique_ptr<TechniqueUniformBlock>>& storages);
    //  Положить данные в _savedValue (когда в _storage не получается)
    void _saveValue(ValueRef data);

  protected:
    const Technique& _technique;
    const TechniqueConfiguration::UniformVariable* _description;

    bool _valueIsSetted;
    //  В нормальных условиях значения переменных хранятся здесь
    TechniqueUniformBlock* _storage;

    //  Здесь хранится имя, если конфигурация не подключена, либо если в
    //  конфигурации нет униформа с таким именем
    std::string _savedName;
    //  Здесь хранится значение, если по какой-то причине не удается
    //  хранить его в _storage
    std::vector<char> _savedValue;
  };

  //  Дополнение для внутреннего использования внутри техники
  class UniformVariableImpl : public UniformVariable
  {
  public:
    inline UniformVariableImpl(
            const char* name,
            const Technique& technique,
            const TechniqueConfiguration* configuration,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks);
    UniformVariableImpl(const UniformVariableImpl&) = delete;
    UniformVariableImpl& operator = (const UniformVariableImpl&) = delete;
    ~UniformVariableImpl() noexcept = default;

    inline void setConfiguration(
            const TechniqueConfiguration* configuration,
            const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks);
  };

  inline const std::string& UniformVariable::name() const noexcept
  {
    if(_description != nullptr) return _description->fullName;
    return _savedName;
  }

  template <typename DataType>
  inline void UniformVariable::setValue(const DataType& data)
  {
    ValueRef valueRef{.data = &data, .dataSize = sizeof(DataType)};
    setValue(valueRef);
  }

  template <typename DataType>
  inline void UniformVariable::setValue(const std::vector<DataType>& data)
  {
    ValueRef valueRef{.data = data.data(),
                      .dataSize = sizeof(DataType) * data.size()};
    setValue(valueRef);
  }

  template <typename DataType>
  inline void UniformVariable::setValue(TechniqueVolatileContext& context,
                                        const DataType& data) const
  {
    ValueRef valueRef{ .data = &data, .dataSize = sizeof(DataType) };
    setValue(context, valueRef);
  }

  template <typename DataType>
  inline void UniformVariable::setValue(TechniqueVolatileContext& context,
                                        const std::vector<DataType>& data) const
  {
    ValueRef valueRef{.data = data.data(),
                      .dataSize = sizeof(DataType) * data.size() };
    setValue(context, valueRef);
  }

  inline UniformVariable::ValueRef UniformVariable::getValue() const noexcept
  {
    if(!_valueIsSetted) return ValueRef{};

    if(!_savedValue.empty())
    {
      return ValueRef{.data = _savedValue .data(),
                      .dataSize = _savedValue.size()};
    }
    MT_ASSERT(_description != nullptr);
    MT_ASSERT(_storage != nullptr);
    return ValueRef{.data = _storage->getData(_description->offsetInBuffer),
                    .dataSize = _description->size};
  }

  inline uint32_t UniformVariable::guiHints() const noexcept
  {
    if (_description != nullptr) return _description->guiHints;
    return 0;
  }

  inline UniformVariableImpl::UniformVariableImpl(
          const char* name,
          const Technique& technique,
          const TechniqueConfiguration* configuration,
          const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks) :
    UniformVariable(name, technique, configuration, blocks)
  {
  }

  inline void UniformVariableImpl::setConfiguration(
              const TechniqueConfiguration* configuration,
              const std::vector<std::unique_ptr<TechniqueUniformBlock>>& blocks)
  {
    _bindToConfiguration(configuration, blocks);
  }
}