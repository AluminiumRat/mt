#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  //  Хелпер для заполнения VkSpecializationInfo и хранения данных для него
  //  Используется для настройки констант специализации для пайплайнов
  class SpecializationInfo
  {
  public:
    SpecializationInfo() noexcept;
    inline SpecializationInfo(const SpecializationInfo& other);
    inline SpecializationInfo(SpecializationInfo&& other) noexcept;
    inline SpecializationInfo& operator = (const SpecializationInfo& other);
    inline SpecializationInfo& operator = (SpecializationInfo&& other) noexcept;
    ~SpecializationInfo() noexcept = default;

    //  Общие методы добавить константу
    void addConstant(uint32_t constantID, const void* data, size_t dataSize);
    inline void addConstant(uint32_t constantID,
                            std::span<const std::byte> data);

    //  Частные методы
    inline void addConstant(uint32_t constantID, uint32_t value);
    inline void addConstant(uint32_t constantID, float value);

    inline const VkSpecializationInfo& vulkanInfo() const noexcept;

  private:
    void _updateVulkanInfo() noexcept;

  private:
    std::vector<VkSpecializationMapEntry> _entryInfo;
    std::vector<std::byte> _entryData;
    VkSpecializationInfo _vulkanInfo;
  };

  inline SpecializationInfo::SpecializationInfo(
                                              const SpecializationInfo& other) :
    _entryInfo(other._entryInfo),
    _entryData(other._entryData),
    _vulkanInfo{}
  {
    _updateVulkanInfo();
  }

  inline SpecializationInfo::SpecializationInfo(
                                          SpecializationInfo&& other) noexcept :
    _entryInfo(std::move(other._entryInfo)),
    _entryData(std::move(other._entryData)),
    _vulkanInfo{}
  {
    _updateVulkanInfo();
  }
  
  inline SpecializationInfo& SpecializationInfo::operator = (
                                                const SpecializationInfo& other)
  {
    _entryInfo = other._entryInfo;
    _entryData = other._entryData;
    _updateVulkanInfo();
  }

  inline SpecializationInfo& SpecializationInfo::operator = (
                                          SpecializationInfo&& other) noexcept
  {
    _entryInfo = std::move(other._entryInfo);
    _entryData = std::move(other._entryData);
    _updateVulkanInfo();
  }

  inline void SpecializationInfo::addConstant(uint32_t constantID,
                                              std::span<const std::byte> data)
  {
    addConstant(constantID, data.data(), data.size());
  }

  inline void SpecializationInfo::addConstant(uint32_t constantID,
                                              uint32_t value)
  {
    addConstant(constantID, &value, sizeof(value));
  }
  
  inline void SpecializationInfo::addConstant(uint32_t constantID,
                                              float value)
  {
    addConstant(constantID, &value, sizeof(value));
  }

  inline const VkSpecializationInfo&
                                SpecializationInfo::vulkanInfo() const noexcept
  {
    return _vulkanInfo;
  }
}