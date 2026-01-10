#pragma once

#include <string>

namespace mt
{
  //  Настройки окна. Используются для сохранения положения и размеров окна
  //  при закрытии и восстановлении при следующем запуске приложения
  struct WindowConfiguration
  {
    int width = 800;
    int height = 600;
    int xPos = 100;
    int yPos = 100;
    bool isMaximized = false;
    bool isMinimized = false;

    std::string imguiConfig;
  };
}