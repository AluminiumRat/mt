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

    inline VkDeviceAddress vkGetBufferDeviceAddress(
                              VkBufferDeviceAddressInfoKHR* addressInfo) const;

    inline VkResult vkGetAccelerationStructureBuildSizesKHR(
                  VkAccelerationStructureBuildTypeKHR buildType,
                  const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
                  const uint32_t* pMaxPrimitiveCounts,
                  VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) const;

    inline VkResult vkCreateAccelerationStructureKHR(
                      const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
                      VkAccelerationStructureKHR* pAccelerationStructure) const;

    inline void vkDestroyAccelerationStructureKHR(
                        VkAccelerationStructureKHR accelerationStructure) const;

    inline VkResult vkCmdBuildAccelerationStructuresKHR(
                      VkCommandBuffer commandBuffer,
                      uint32_t infoCount,
                      const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
                      const VkAccelerationStructureBuildRangeInfoKHR*
                                                const* ppBuildRangeInfos) const;

    inline VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(
                      VkAccelerationStructureDeviceAddressInfoKHR* pInfo) const;

  private:
    PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT = nullptr;
    PFN_vkGetBufferDeviceAddress _vkGetBufferDeviceAddress = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR
                            _vkGetAccelerationStructureBuildSizesKHR = nullptr;
    PFN_vkCreateAccelerationStructureKHR
                                    _vkCreateAccelerationStructureKHR = nullptr;
    PFN_vkDestroyAccelerationStructureKHR
                                  _vkDestroyAccelerationStructureKHR = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR
                                _vkCmdBuildAccelerationStructuresKHR = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR
                          _vkGetAccelerationStructureDeviceAddressKHR = nullptr;
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

  inline VkDeviceAddress ExtFunctions::vkGetBufferDeviceAddress(
                                VkBufferDeviceAddressInfoKHR* addressInfo) const
  {
    return _vkGetBufferDeviceAddress(_device, addressInfo);
  }

  inline VkResult ExtFunctions::vkGetAccelerationStructureBuildSizesKHR(
                VkAccelerationStructureBuildTypeKHR buildType,
                const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
                const uint32_t* pMaxPrimitiveCounts,
                VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) const
  {
    if(_vkGetAccelerationStructureBuildSizesKHR == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    _vkGetAccelerationStructureBuildSizesKHR( _device,
                                              buildType,
                                              pBuildInfo,
                                              pMaxPrimitiveCounts,
                                              pSizeInfo);

    return VK_SUCCESS;
  }

  inline VkResult ExtFunctions::vkCreateAccelerationStructureKHR(
                        const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
                        VkAccelerationStructureKHR* pAccelerationStructure) const
  {
    if(_vkCreateAccelerationStructureKHR == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return _vkCreateAccelerationStructureKHR( _device,
                                              pCreateInfo,
                                              nullptr,
                                              pAccelerationStructure);
  }

  inline void ExtFunctions::vkDestroyAccelerationStructureKHR(
                        VkAccelerationStructureKHR accelerationStructure) const
  {
    _vkDestroyAccelerationStructureKHR( _device,
                                        accelerationStructure,
                                        nullptr);
  }

  inline VkResult ExtFunctions::vkCmdBuildAccelerationStructuresKHR(
      VkCommandBuffer commandBuffer,
      uint32_t infoCount,
      const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
      const VkAccelerationStructureBuildRangeInfoKHR*
                                                const* ppBuildRangeInfos) const
  {
    if(_vkCmdBuildAccelerationStructuresKHR == nullptr)
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    _vkCmdBuildAccelerationStructuresKHR( commandBuffer,
                                          infoCount,
                                          pInfos,
                                          ppBuildRangeInfos);

    return VK_SUCCESS;
  }

  inline VkDeviceAddress
            ExtFunctions::vkGetAccelerationStructureDeviceAddressKHR(
                      VkAccelerationStructureDeviceAddressInfoKHR* pInfo) const
  {
    if(_vkGetAccelerationStructureDeviceAddressKHR == nullptr) return 0;
    return _vkGetAccelerationStructureDeviceAddressKHR(_device, pInfo);
  }
}