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

  _vkGetBufferDeviceAddress =
          (PFN_vkGetBufferDeviceAddress)
                              vkGetDeviceProcAddr(device,
                                                  "vkGetBufferDeviceAddress");

  _vkGetAccelerationStructureBuildSizesKHR =
    (PFN_vkGetAccelerationStructureBuildSizesKHR)
                vkGetDeviceProcAddr(device,
                                    "vkGetAccelerationStructureBuildSizesKHR");

  _vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)
                        vkGetDeviceProcAddr(device,
                                            "vkCreateAccelerationStructureKHR");

  _vkDestroyAccelerationStructureKHR =
        (PFN_vkDestroyAccelerationStructureKHR)
                      vkGetDeviceProcAddr(device,
                                          "vkDestroyAccelerationStructureKHR");

  _vkCmdBuildAccelerationStructuresKHR =
        (PFN_vkCmdBuildAccelerationStructuresKHR)
                      vkGetDeviceProcAddr(device,
                                        "vkCmdBuildAccelerationStructuresKHR");
}
