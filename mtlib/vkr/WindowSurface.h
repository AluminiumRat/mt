#pragma once

#include <vulkan/vulkan.h>

#include <vkr/PhysicalDevice.h>

namespace mt
{
  // Очень тонкая RAII обертка для VkSurfaceKHR.
  // Можно использовать саму по себе, если вы смогли раздобыть где-то
  // VkSurfaceKHR, либо использовать платформенно-зависимый потомок
  class WindowSurface
  {
  public:
    // handle будет уничтожен в деструкторе
    WindowSurface(VkSurfaceKHR handle) noexcept;
    WindowSurface(const WindowSurface&) = delete;
    WindowSurface& operator = (const WindowSurface&) = delete;
    ~WindowSurface() noexcept;

    inline VkSurfaceKHR handle() const noexcept;

  private:
    void _cleanup() noexcept;

  private:
    VkSurfaceKHR _handle;
  };

  inline VkSurfaceKHR WindowSurface::handle() const noexcept
  {
    return _handle;
  }
}