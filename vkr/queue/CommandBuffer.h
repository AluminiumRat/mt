#pragma once

#include <vector>

#include <vulkan/vulkan.h>

//#include <mtt/render/CommandQueue/ScaledVolatileDescriptorPool.h>
#include <vkr/RefCounter.h>
#include <vkr/Ref.h>

namespace mt
{
  class Device;

  class CommandBuffer : public RefCounter
  {
  public:
    CommandBuffer(VkCommandPool pool,
                  Device& device,
                  VkCommandBufferLevel level);
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator = (const CommandBuffer&) = delete;
  protected:
    virtual ~CommandBuffer();

  public:
    inline VkCommandBuffer handle() const;
    inline VkCommandBufferLevel level() const;
    inline void lockResource(RefCounter& resource);
    inline void releaseResources();
    //inline ScaledVolatileDescriptorPool& volatileDescriptorPool() noexcept;

  private:
    void _cleanup() noexcept;

  private:
    VkCommandBuffer _handle;
    VkCommandPool _pool;
    Device& _device;
    VkCommandBufferLevel _level;

    std::vector<RefCounterReference> _lockedResources;

    //ScaledVolatileDescriptorPool _volatileDescriptorPool;
  };

  inline VkCommandBuffer CommandBuffer::handle() const
  {
    return _handle;
  }

  inline VkCommandBufferLevel CommandBuffer::level() const
  {
    return _level;
  }

  inline void CommandBuffer::lockResource(RefCounter& resource)
  {
    _lockedResources.push_back(RefCounterReference(&resource));
  }

  inline void CommandBuffer::releaseResources()
  {
    _lockedResources.clear();
  }

  /*
  inline ScaledVolatileDescriptorPool&
                                CommandBuffer::volatileDescriptorPool() noexcept
  {
    return _volatileDescriptorPool;
  }*/
}