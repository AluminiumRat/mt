#pragma once

#include <vector>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vulkan/vulkan.h>

namespace mt
{
  class DataBuffer;
  class DescriptorPool;
  class Device;

  class DescriptorSet : public RefCounter
  {
  protected:
    // Создавать дескриптер сеты можно только через пул
    friend class DescriptorPool;
    DescriptorSet(Device& device,
                  VkDescriptorSet handle,
                  const DescriptorPool* pool);
    DescriptorSet(const DescriptorSet&) = delete;
    DescriptorSet& operator = (const DescriptorSet&) = delete;
    virtual ~DescriptorSet() noexcept = default;

  public:
    inline Device& device() const noexcept;
    inline VkDescriptorSet handle() const noexcept;

    void attachUniformBuffer(const DataBuffer& buffer, uint32_t binding);
    void attachStorageBuffer(const DataBuffer& buffer, uint32_t binding);

  private:
    Device& _device;
    VkDescriptorSet _handle;

    std::vector<ConstRef<RefCounter>> _resources;
  };

  inline Device& DescriptorSet::device() const noexcept
  {
    return _device;
  }

  inline VkDescriptorSet DescriptorSet::handle() const noexcept
  {
    return _handle;
  }
}