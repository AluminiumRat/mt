#pragma once

#include <cstring>
#include <string>

#include <technique/TechniqueConfiguration.h>
#include <util/Log.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/queue/PushConstantBlock.h>

namespace mt
{
  //  Компонент класса Technique
  //  Единичная именованная запись в блоке пуш констант. Это могут быть
  //    отдельные переменные, структуры или массивы.
  //  В отличие от других компонентов техники, таких как ResourceBinding или 
  //    UniformVariable, PushConstant не хранит значение. Она предназначена для
  //    немедленного применения, когда техника уже прибинжена к
  //    CommandProducer-у.
  //  Позволяет менять данные, используемые в шейдерах, не перебинживая технику.
  //  Публичный интерфейс для использования снаружи Technique
  class PushConstant
  {
  public:
    PushConstant( const char* name,
                  const TechniqueConfiguration* configuration);
    PushConstant(const PushConstant&) = delete;
    PushConstant& operator = (const PushConstant&) = delete;
    ~PushConstant() noexcept = default;

    inline const std::string& name() const noexcept;

    //  Записать какое-то значение внутрь блока пуш констант, используя информацию
    //  об расположении пуш константы
    template <typename DataType>
    inline void setValue( const DataType& data,
                          PushConstantBlock& target) noexcept;
    //  Записать какое-то значение внутрь блока пуш констант, используя информацию
    //  об расположении пуш константы
    template <typename DataType>
    inline void setValue( const std::vector<DataType>& data,
                          PushConstantBlock& target) noexcept;

    //  Хэлпер, который позволяет записать несколько пуш констант в один вызов
    template <typename ... Params>
    inline static void pushTogether(CommandProducerCompute& producer,
                                    Params&&... params);

  protected:
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  private:
    //  Запчасти для pushTogether
    template <typename DataType, typename ... Params>
    inline static void _setValueTogether( PushConstantBlock& target,
                                          PushConstant& constant,
                                          const DataType& data,
                                          Params&&... params) noexcept;
    //  Запчасти для pushTogether
    template <typename DataType>
    inline static void _setValueTogether( PushConstantBlock& target,
                                          PushConstant& constant,
                                          const DataType& data) noexcept;
  private:
    const TechniqueConfiguration::PushConstant* _description;
    std::string _name;
  };

  //  Дополнение для внутреннего использования внутри техники
  class PushConstantImpl : public PushConstant
  {
  public:
    inline PushConstantImpl(const char* name,
                            const TechniqueConfiguration* configuration);
    PushConstantImpl(const PushConstantImpl&) = delete;
    PushConstantImpl& operator = (const PushConstantImpl&) = delete;
    ~PushConstantImpl() noexcept = default;

    inline void setConfiguration(const TechniqueConfiguration* configuration);
  };

  inline const std::string& PushConstant::name() const noexcept
  {
    return _name;
  }

  template <typename DataType>
  inline void PushConstant::setValue( const DataType& data,
                                      PushConstantBlock& target) noexcept
  {
    if(_description == nullptr) return;
    if(_description->size < sizeof(data))
    {
      Log::warning() << _description->fullName << ": data size is bigger than push constant size";
      return;
    }
    std::memcpy(target.data() + _description->offset, &data, sizeof(data));
  }

  template <typename DataType>
  inline void PushConstant::setValue( const std::vector<DataType>& data,
                                      PushConstantBlock& target) noexcept
  {
    if (_description == nullptr) return;

    size_t dataSize = sizeof(DataType) * data.size();
    if (_description->size < dataSize)
    {
      Log::warning() << _description->fullName << ": data size is bigger than push constant size";
      return;
    }

    std::memcpy(target.data() + _description->offset, data.data(), dataSize);
  }

  template <typename DataType>
  inline static void PushConstant::_setValueTogether(
                                                  PushConstantBlock& target,
                                                  PushConstant& constant,
                                                  const DataType& data) noexcept
  {
    constant.setValue(data, target);
  }

  template <typename DataType, typename ... Params>
  inline static void PushConstant::_setValueTogether(
                                                    PushConstantBlock& target,
                                                    PushConstant& constant,
                                                    const DataType& data,
                                                    Params&&... params) noexcept
  {
    constant.setValue(data, target);
    _setValueTogether(target, std::forward<Params>(params)...);
  }

  template <typename ... Params>
  inline void PushConstant::pushTogether( CommandProducerCompute& producer,
                                          Params&&... params)
  {
    PushConstantBlock pushConstants;
    _setValueTogether(pushConstants, std::forward<Params>(params)...);
    producer.pushConstants(pushConstants);
  }

  inline PushConstantImpl::PushConstantImpl(
                                  const char* name,
                                  const TechniqueConfiguration* configuration) :
    PushConstant(name, configuration)
  {
  }

  inline void PushConstantImpl::setConfiguration(
                                    const TechniqueConfiguration* configuration)
  {
    _bindToConfiguration(configuration);
  }
}