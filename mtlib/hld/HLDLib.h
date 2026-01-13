#pragma once

#include <atomic>

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

    inline uint32_t getStageIndex(const std::string& stageName);
    inline uint32_t getFrameTypeIndex(const std::string& frameTypeName);

    //  Создать новый уникальный groupIndex для использования в DrawCommand
    inline uint32_t allocateDrawCommandGroupIndex() noexcept;

  private:
    static HLDLib* _instance;

    StringRegistry _frameTypeRegistry;
    StringRegistry _stagesRegistry;
    std::atomic<uint32_t> _drawCommandGroupCount;
  };

  inline HLDLib& HLDLib::instance() noexcept
  {
    MT_ASSERT(_instance != nullptr);
    return *_instance;
  }

  inline uint32_t HLDLib::getFrameTypeIndex(const std::string& frameTypeName)
  {
    return (uint32_t)_frameTypeRegistry.getIndex(frameTypeName);
  }

  inline uint32_t HLDLib::getStageIndex(const std::string& stageName)
  {
    return (uint32_t)_stagesRegistry.getIndex(stageName);
  }

  inline uint32_t HLDLib::allocateDrawCommandGroupIndex() noexcept
  {
    return _drawCommandGroupCount++;
  }
}