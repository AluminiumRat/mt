#include <vkr/ExtFunctions.h>

using namespace mt;

ExtFunctions::ExtFunctions(VkDevice device) noexcept
{
  _device = device;

  _vkSetDebugUtilsObjectNameEXT =
          (PFN_vkSetDebugUtilsObjectNameEXT)
                            vkGetDeviceProcAddr(device,
                                                "vkSetDebugUtilsObjectNameEXT");

  _vkCmdBeginDebugUtilsLabelEXT =
          (PFN_vkCmdBeginDebugUtilsLabelEXT)
                            vkGetDeviceProcAddr(device,
                                                "vkCmdBeginDebugUtilsLabelEXT");

  _vkCmdEndDebugUtilsLabelEXT =
          (PFN_vkCmdEndDebugUtilsLabelEXT)
                            vkGetDeviceProcAddr(device,
                                                  "vkCmdEndDebugUtilsLabelEXT");
}
