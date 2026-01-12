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

    inline VkResult vkCmdBeginDebugUtilsLabelEXT(
                                  VkCommandBuffer commandBuffer,
                                  const VkDebugUtilsLabelEXT* pLabelInfo) const;

    inline VkResult vkCmdEndDebugUtilsLabelEXT(
                                          VkCommandBuffer commandBuffer) const;

  private:
    PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT = nullptr;
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

  inline VkResult ExtFunctions::vkCmdBeginDebugUtilsLabelEXT(
                                  VkCommandBuffer commandBuffer,
                                  const VkDebugUtilsLabelEXT* pLabelInfo) const
  {
    if(_vkCmdBeginDebugUtilsLabelEXT == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    _vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
    return VK_SUCCESS;
  }

  inline VkResult ExtFunctions::vkCmdEndDebugUtilsLabelEXT(
                                          VkCommandBuffer commandBuffer) const
  {
    if (_vkCmdEndDebugUtilsLabelEXT == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    _vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    return VK_SUCCESS;
  }
}