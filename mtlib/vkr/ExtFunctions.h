#pragma once

#include <vulkan/vulkan.h>

namespace mt
{
  // Набор функций вулкана, которые не прогружаются дефолтным лоадером
  class ExtFunctions
  {
  public:
    ExtFunctions(VkDevice device) noexcept;
    ExtFunctions(const ExtFunctions&) noexcept = default;
    ExtFunctions& operator = (const ExtFunctions&) noexcept = default;
    ~ExtFunctions() noexcept = default;

    inline VkResult vkSetDebugUtilsObjectNameEXT(
                          const VkDebugUtilsObjectNameInfoEXT* pNameInfo) const;

  private:
    PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;
    VkDevice _device = VK_NULL_HANDLE;
  };

  inline VkResult ExtFunctions::vkSetDebugUtilsObjectNameEXT(
                          const VkDebugUtilsObjectNameInfoEXT* pNameInfo) const
  {
    if(_vkSetDebugUtilsObjectNameEXT == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return _vkSetDebugUtilsObjectNameEXT(_device, pNameInfo);
  }
}