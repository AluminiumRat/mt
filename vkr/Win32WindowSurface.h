#pragma once

#ifdef WIN32

#include <windows.h>

#include <vkr/WindowSurface.h>

namespace mt
{
  // Специальная версия WindowSurface, которая умеет конструироваться из HWND
  // Вынесено в отдельный файл, чтобы не тащить windows-заголовки (нужно
  // определение HWND) внутрь рендер библиотеки
  class Win32WindowSurface : public WindowSurface
  {
  public:
    Win32WindowSurface(HWND hwnd);
    Win32WindowSurface(const Win32WindowSurface&) = delete;
    Win32WindowSurface& operator = (const Win32WindowSurface&) = delete;
    ~Win32WindowSurface() noexcept = default;
  };
}

#endif