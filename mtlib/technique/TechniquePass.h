#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace mt
{
  class Technique;
  struct TechniqueConfiguration;

  //  Компонент класса Technique
  //  Объект, который позволяет выбрать, какой проход будет использоваться
  //    в технике при рисовании/вычислении
  //  Публичный интерфейс для использования снаружи Technique
  class TechniquePass
  {
  public:
    TechniquePass(const char* name,
                  const TechniqueConfiguration* configuration);
    TechniquePass(const TechniquePass&) = delete;
    TechniquePass& operator = (const TechniquePass&) = delete;
    ~TechniquePass() noexcept = default;

    inline const std::string& name() const noexcept;

  protected:
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    std::string _name;
    uint32_t _passIndex;
  };

  //  Дополнение для внутреннего использования внутри техники
  class TechniquePassImpl : public TechniquePass
  {
  public:
    //  Если метод passIndex возвращает это значение, значит в технике нет
    //  прохода с этим именем
    static constexpr uint32_t NOT_PASS_INDEX =
                                          std::numeric_limits<uint32_t>::max();
  public:
    inline TechniquePassImpl( const char* name,
                              const TechniqueConfiguration* configuration);
    TechniquePassImpl(const TechniquePassImpl&) = delete;
    TechniquePassImpl& operator = (const TechniquePassImpl&) = delete;
    ~TechniquePassImpl() noexcept = default;

    inline uint32_t passIndex() const noexcept;

    inline void setConfiguration(const TechniqueConfiguration* configuration);
  };

  inline const std::string& TechniquePass::name() const noexcept
  {
    return _name;
  }

  inline TechniquePassImpl::TechniquePassImpl(
                                  const char* name,
                                  const TechniqueConfiguration* configuration) :
    TechniquePass(name, configuration)
  {
  }

  inline uint32_t TechniquePassImpl::passIndex() const noexcept
  {
    return _passIndex;
  }

  inline void TechniquePassImpl::setConfiguration(
                                    const TechniqueConfiguration* configuration)
  {
    _bindToConfiguration(configuration);
  }
}