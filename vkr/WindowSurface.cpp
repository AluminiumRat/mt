#include <stdexcept>

#include <util/util.h>
#include <vkr/VKRLib.h>
#include <vkr/WindowSurface.h>

using namespace mt;

WindowSurface::WindowSurface(VkSurfaceKHR handle) noexcept :
  _handle(handle)
{
  MT_ASSERT((_handle != VK_NULL_HANDLE) && "WindowSurface: null handle is received");
}

WindowSurface::~WindowSurface() noexcept
{
  _cleanup();
}

void WindowSurface::_cleanup() noexcept
{
  if (_handle != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(VKRLib::instance().handle(),
                        _handle,
                        nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
