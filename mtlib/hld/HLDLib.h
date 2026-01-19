#pragma once

#include <atomic>

#include <hld/drawCommand/DrawCommand.h>
#include <hld/FrameTypeIndex.h>
#include <hld/StageIndex.h>
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

    //  Получить индлекс стадии отрисовки по её имени
    inline StageIndex getStageIndex(const std::string& stageName);

    //  Получить тип фрэйма по его имени
    inline FrameTypeIndex getFrameTypeIndex(const std::string& frameTypeName);

    //  Создать новый уникальный groupIndex для использования в DrawCommand
    inline DrawCommand::Group allocateDrawCommandGroup() noexcept;

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

  inline FrameTypeIndex HLDLib::getFrameTypeIndex(
                                              const std::string& frameTypeName)
  {
    return (FrameTypeIndex)_frameTypeRegistry.getIndex(frameTypeName);
  }

  inline StageIndex HLDLib::getStageIndex(const std::string& stageName)
  {
    return (StageIndex)_stagesRegistry.getIndex(stageName);
  }

  inline DrawCommand::Group HLDLib::allocateDrawCommandGroup() noexcept
  {
    return _drawCommandGroupCount++;
  }
}