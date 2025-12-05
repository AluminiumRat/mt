#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <gui/WindowConfiguration.h>
#include <util/Assert.h>
#include <vkr/Device.h>

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
    GUILib();
    GUILib(const GUILib&) = delete;
    GUILib& operator = (const GUILib&) = delete;
    ~GUILib() noexcept;

    // Нельзя вызывать до создания GUILib
    inline static GUILib& instance() noexcept;

    //  Загрузить настройки положений и размеров окон из конфигурационного файла
    //  Не влияет на уже созданные окна. Новые настройки применяются только
    //    к вновь создаваемым окнам.
    //  В случае каких-либо ошибок этот метод пишет warning-и в лог, а не
    //    выбрасывает исключения
    void loadConfiguration(const std::filesystem::path& file) noexcept;
    //  Сохранить в файл настройки положений и размеров окон
    //  В случае каких-либо ошибок этот метод пишет warning-и в лог, а не
    //    выбрасывает исключения
    void saveConfiguration(const std::filesystem::path& file) const noexcept;

    void updateWindows();
    void drawWindows();

    //  Проверяет, есть ли ещё не закрытые окна. Возвращает true, если все окна
    //    закрыты
    bool shouldBeClosed() const noexcept;

    //  Создать устройство, подходящее для рендера в окно
    std::unique_ptr<Device> createDevice(
                            VkPhysicalDeviceFeatures requiredFeatures,
                            const std::vector<std::string>& requiredExtensions,
                            QueuesConfiguration configuration);
  private:
    friend class BaseWindow;
    inline void registerWindow(BaseWindow& window);
    inline void unregisterWindow(BaseWindow& window) noexcept;
    inline const WindowConfiguration* getConfiguration(
                                        const std::string& windowName) noexcept;
    inline void addConfiguration( const std::string& windowName,
                                  const WindowConfiguration& cofiguration);

  private:
    static GUILib* _instance;

    using Windows = std::vector<BaseWindow*>;
    Windows _windows;

    //  Настройки для окон. Используются для восстановления состояния
    //  после закрытия и нового запуска приложения
    using ComfigurationMap = std::map<std::string, WindowConfiguration>;
    ComfigurationMap _configurationMap;
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

  inline const WindowConfiguration* GUILib::getConfiguration(
                                        const std::string& windowName) noexcept
  {
    ComfigurationMap::const_iterator iConfig =
                                            _configurationMap.find(windowName);
    if(iConfig == _configurationMap.end()) return nullptr;
    return &iConfig->second;
  }

  inline void GUILib::addConfiguration( const std::string& windowName,
                                        const WindowConfiguration& cofiguration)
  {
    if(windowName.empty()) return;
    _configurationMap[windowName] = cofiguration;
  }
}