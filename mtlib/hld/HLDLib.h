#pragma once

#include <util/Assert.h>
#include <util/StringRegistry.h>

namespace mt
{
  //  Синглтон для библиотеки высокоуровневого рендера через вулкан
  //  Объект класса должен быть явно создан через конструктор и существовать
  //    на протяжении всего времени работы с рендером.
  //  Доступ к объекту можно производить через метод instance, но только после
  //    явного создания объекта
  class HLDLib
  {
  public:
    HLDLib();
    HLDLib(const HLDLib&) = delete;
    HLDLib& operator = (const HLDLib&) = delete;
    ~HLDLib() noexcept;

    inline static HLDLib& instance() noexcept;

    inline size_t getStageIndex(const std::string& stageName);

  private:
    static HLDLib* _instance;

    StringRegistry _stagesRegistry;
  };

  inline HLDLib& HLDLib::instance() noexcept
  {
    MT_ASSERT(_instance != nullptr);
    return *_instance;
  }

  inline size_t HLDLib::getStageIndex(const std::string& stageName)
  {
    return _stagesRegistry.getIndex(stageName);
  }
}