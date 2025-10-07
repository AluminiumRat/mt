#ifdef WIN32

#define VK_USE_PLATFORM_WIN32_KHR

#include <stdexcept>

#include <vkr/VKRLib.h>
#include <vkr/Win32WindowSurface.h>

using namespace mt;

static VkSurfaceKHR createSurfaceHandle(HWND hwnd)
{
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hwnd = hwnd;
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

  VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;
  if(vkCreateWin32SurfaceKHR( VKRLib::instance().handle(),
                              &surfaceCreateInfo,
                              nullptr,
                              &surfaceHandle) != VK_SUCCESS)
  {
    throw std::runtime_error("Win32WindowSurface: Failed to create render surface from hwnd");
  }

  return surfaceHandle;
}

Win32WindowSurface::Win32WindowSurface(HWND hwnd) :
  WindowSurface(createSurfaceHandle(hwnd))
{
}

#endif