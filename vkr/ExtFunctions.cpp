#include <vkr/ExtFunctions.h>

using namespace mt;

ExtFunctions::ExtFunctions(VkDevice device) noexcept
{
  _device = device;

  _vkSetDebugUtilsObjectNameEXT =
          (PFN_vkSetDebugUtilsObjectNameEXT)
                            vkGetDeviceProcAddr(device,
                                                "vkSetDebugUtilsObjectNameEXT");
}
