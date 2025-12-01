#pragma once

#include <string>
#include <vector>

#include <util/Assert.h>

namespace mt
{
  class BaseWindow;

  //  Синглтон для GUI библиотеки.
  //  Объект класса должен быть явно создан через конструктор и существовать
  //    на протяжении всего времени работы с gui системой.
  //  Доступ к объекту можно производить через метод instance, но только после
  //    явного создания объекта
  //  Библиотека GUI строится поверх VKR, поэтому экземпляр VKRLib должен быть
  //    создан до GUILib, а уничтожен после уничтожения GUILib
  class GUILib
  {
  public:
    GUILib(const char* configFilename);
    GUILib(const GUILib&) = delete;
    GUILib& operator = (const GUILib&) = delete;
    ~GUILib() noexcept;

    // Нельзя вызывать до создания GUILib
    inline static GUILib& instance() noexcept;

    void updateWindows();
    void drawWindows();

  private:
    friend class BaseWindow;
    inline void registerWindow(BaseWindow& window);
    inline void unregisterWindow(BaseWindow& window) noexcept;

  private:
    static GUILib* _instance;

    std::string _configFilename;

    using Windows = std::vector<BaseWindow*>;
    Windows _windows;
  };

  inline GUILib& GUILib::instance() noexcept
  {
    MT_ASSERT(_instance != nullptr);
    return *_instance;
  }

  inline void GUILib::registerWindow(BaseWindow& window)
  {
    _windows.push_back(&window);
  }

  inline void GUILib::unregisterWindow(BaseWindow& window) noexcept
  {
    for(Windows::iterator iWindow = _windows.begin();
        iWindow != _windows.end();
        iWindow++)
    {
      if(*iWindow == &window)
      {
        _windows.erase(iWindow);
        return;
      }
    }
  }
}