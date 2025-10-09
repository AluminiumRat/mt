#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/QueueSources.h>
#include <vkr/queue/QueueTypes.h>
//#include <mtt/render/CommandQueue/CommandProducer.h>

namespace mt
{
  class PhysicalDevice;
  class RenderSurface;

  // Логическое устройство, обертка вокруг вулкановского VkDevice
  // Основной класс, через который происходит взаимодействие с видеокартой
  // В отличие от PhysicalDevice, которые непосредственно представляют
  // видекарты установленные в системе, логическое устройство - это интерфейс
  // для взаимодействия с физическим устройством. Для одного PhysicalDevice
  // можно создать несколько независимых Device (зачем-бы?).
  class Device
  {
  private:
    friend class VKRLib;
    // Логическое устройство можно создать только через класс VKRLib
    Device( PhysicalDevice& physicalDevice,
            VkPhysicalDeviceFeatures requiredFeatures,
            const std::vector<std::string>& requiredExtensions,
            const std::vector<std::string>& enabledLayers,
            const QueueSources& queueSources);

  public:
    Device(const Device&) = delete;
    Device& operator = (const Device&) = delete;
    ~Device() noexcept;

    inline VkDevice handle() const noexcept;
    inline VmaAllocator allocator() noexcept;

    inline const VkPhysicalDeviceFeatures& features() const noexcept;

    inline PhysicalDevice& physicalDevice() const noexcept;

    inline CommandQueue* graphicQueue() noexcept;
    inline const CommandQueue* graphicQueue() const noexcept;

    inline CommandQueue* computeQueue() noexcept;
    inline const CommandQueue* computeQueue() const noexcept;

    inline CommandQueue* presentationQueue() noexcept;
    inline const CommandQueue* presentationQueue() const noexcept;

    inline CommandQueue* transferQueue() noexcept;
    inline const CommandQueue* transferQueue() const noexcept;

    /// Synchronized access to the separate transfer command queue.
    /// The command will executed on the GPU immediately and control
    /// will returned after execution.
    /// command is callable object with signature void(CommandProducer&)
    //template<typename TransferCommand>
    //inline void runTransferCommand(TransferCommand command);

    bool isSurfaceSuitable(const RenderSurface& surface) const;

  private:
    void _cleanup() noexcept;
    void _createHandle( const std::vector<std::string>& requiredExtensions,
                        const std::vector<std::string>& enabledLayers,
                        const QueueSources& queueSources);
    void _initVMA();
    void _buildQueues(const QueueSources& queueSources);

  private:
    PhysicalDevice& _physicalDevice;
    VkDevice _handle;

    VmaAllocator _allocator;

    VkPhysicalDeviceFeatures _features;

    // Общий мьютекс для очередей. Добавлен так как очереди поддерживают
    // многопоток и под капотом могут взаимодействовать друг с другом.
    std::mutex _commonQueuesMutex;
    std::vector<std::unique_ptr<CommandQueue>> _queues;
    std::array<CommandQueue*, QueueTypeCount> _queuesByTypes;
  };

  inline VkDevice Device::handle() const noexcept
  {
    return _handle;
  }

  inline VmaAllocator Device::allocator() noexcept
  {
    return _allocator;
  }

  inline const VkPhysicalDeviceFeatures& Device::features() const noexcept
  {
    return _features;
  }

  inline PhysicalDevice& Device::physicalDevice() const noexcept
  {
    return _physicalDevice;
  }

  inline CommandQueue* Device::graphicQueue() noexcept
  {
    return _queuesByTypes[GRAPHIC_QUEUE];
  }

  inline const CommandQueue* Device::graphicQueue() const noexcept
  {
    return _queuesByTypes[GRAPHIC_QUEUE];
  }

  inline CommandQueue* Device::computeQueue() noexcept
  {
    return _queuesByTypes[COMPUTE_QUEUE];
  }

  inline const CommandQueue* Device::computeQueue() const noexcept
  {
    return _queuesByTypes[COMPUTE_QUEUE];
  }

  inline CommandQueue* Device::presentationQueue() noexcept
  {
    return _queuesByTypes[PRESENTATION_QUEUE];
  }

  inline const CommandQueue* Device::presentationQueue() const noexcept
  {
    return _queuesByTypes[PRESENTATION_QUEUE];
  }

  inline CommandQueue* Device::transferQueue() noexcept
  {
    return _queuesByTypes[TRANSFER_QUEUE];
  }

  inline const CommandQueue* Device::transferQueue() const noexcept
  {
    return _queuesByTypes[TRANSFER_QUEUE];
  }

  /*template<typename TransferCommand>
  inline void Device::runTransferCommand(TransferCommand command)
  {
    std::lock_guard lock(_transferQueueMutex);
    std::unique_ptr<CommandProducer> producer = _transferQueue->startCommands();
    command(*producer);
    _transferQueue->submit( std::move(producer),
                            nullptr,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            nullptr);
    _transferQueue->waitIdle();
  }*/
}