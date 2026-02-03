#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <util/Assert.h>
#include <vkr/queue/CommandQueue.h>
#include <vkr/queue/CommandQueueCompute.h>
#include <vkr/queue/CommandQueueGraphic.h>
#include <vkr/queue/CommandQueueTransfer.h>
#include <vkr/queue/QueueSources.h>
#include <vkr/queue/QueueTypes.h>
#include <vkr/ExtFunctions.h>
#include <vkr/PhysicalDevice.h>

namespace mt
{
  class WindowSurface;

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
            const PhysicalDevice::Features& requiredFeatures,
            const std::vector<std::string>& requiredExtensions,
            const std::vector<std::string>& enabledLayers,
            const QueueSources& queueSources);

  public:
    Device(const Device&) = delete;
    Device& operator = (const Device&) = delete;
    ~Device() noexcept;

    inline VkDevice handle() const noexcept;
    inline VmaAllocator allocator() noexcept;

    inline const PhysicalDevice::Features& features() const noexcept;

    inline PhysicalDevice& physicalDevice() const noexcept;

    inline CommandQueueGraphic* graphicQueue() noexcept;
    inline const CommandQueueGraphic* graphicQueue() const noexcept;

    inline CommandQueueCompute& computeQueue() noexcept;
    inline const CommandQueueCompute& computeQueue() const noexcept;

    inline CommandQueue* presentationQueue() noexcept;
    inline const CommandQueue* presentationQueue() const noexcept;

    inline CommandQueueTransfer& transferQueue() noexcept;
    inline const CommandQueueTransfer& transferQueue() const noexcept;

    // Очередь, через которую происходит основная работа на GPU
    // Обычно это graphicQueue, но если пользователь сказал не создавать
    // графическую очередь, то основной становится compute
    inline CommandQueueCompute& primaryQueue() noexcept;
    inline const CommandQueueCompute& primaryQueue() const noexcept;

    bool isSurfaceSuitable(const WindowSurface& surface) const;

    //  Заблокировать выполнение чего-либо на GPU очередях
    //  Метод блокирует доступ к очередям со стороны других потоков, дожидается,
    //    когда все очереди закончат выполнение текущих команд, после чего
    //    возвращает std::unique_lock на общий мьютекс очередей, который
    //    не дает другим потокам добавлять новые команды в очередь
    std::unique_lock<std::recursive_mutex> lockQueues() noexcept;

    inline const ExtFunctions& extFunctions() const noexcept;

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

    PhysicalDevice::Features _features;

    // Общий мьютекс для очередей. Добавлен так как очереди поддерживают
    // многопоток и под капотом могут взаимодействовать друг с другом.
    std::recursive_mutex _commonQueuesMutex;
    std::vector<std::unique_ptr<CommandQueue>> _queues;
    std::array<CommandQueue*, QueueTypeCount> _queuesByTypes;

    // std::optional для отложенного создания в конце конструктора
    std::optional<ExtFunctions> _extFunctions;
  };

  inline VkDevice Device::handle() const noexcept
  {
    return _handle;
  }

  inline VmaAllocator Device::allocator() noexcept
  {
    return _allocator;
  }

  inline const PhysicalDevice::Features& Device::features() const noexcept
  {
    return _features;
  }

  inline PhysicalDevice& Device::physicalDevice() const noexcept
  {
    return _physicalDevice;
  }

  inline CommandQueueGraphic* Device::graphicQueue() noexcept
  {
    return static_cast<CommandQueueGraphic*>(_queuesByTypes[GRAPHIC_QUEUE]);
  }

  inline const CommandQueueGraphic* Device::graphicQueue() const noexcept
  {
    return static_cast<CommandQueueGraphic*>(_queuesByTypes[GRAPHIC_QUEUE]);
  }

  inline CommandQueueCompute& Device::computeQueue() noexcept
  {
    MT_ASSERT(_queuesByTypes[COMPUTE_QUEUE] != nullptr);
    return *static_cast<CommandQueueCompute*>(_queuesByTypes[COMPUTE_QUEUE]);
  }

  inline const CommandQueueCompute& Device::computeQueue() const noexcept
  {
    MT_ASSERT(_queuesByTypes[COMPUTE_QUEUE] != nullptr);
    return *static_cast<CommandQueueCompute*>(_queuesByTypes[COMPUTE_QUEUE]);
  }

  inline CommandQueue* Device::presentationQueue() noexcept
  {
    return _queuesByTypes[PRESENTATION_QUEUE];
  }

  inline const CommandQueue* Device::presentationQueue() const noexcept
  {
    return _queuesByTypes[PRESENTATION_QUEUE];
  }

  inline CommandQueueTransfer& Device::transferQueue() noexcept
  {
    MT_ASSERT(_queuesByTypes[TRANSFER_QUEUE] != nullptr);
    return *static_cast<CommandQueueTransfer*>(_queuesByTypes[TRANSFER_QUEUE]);
  }

  inline const CommandQueueTransfer& Device::transferQueue() const noexcept
  {
    MT_ASSERT(_queuesByTypes[TRANSFER_QUEUE] != nullptr);
    return *static_cast<CommandQueueTransfer*>(_queuesByTypes[TRANSFER_QUEUE]);
  }

  inline CommandQueueCompute& Device::primaryQueue() noexcept
  {
    if(_queuesByTypes[GRAPHIC_QUEUE] != nullptr)
    {
      return *static_cast<CommandQueueCompute*>(_queuesByTypes[GRAPHIC_QUEUE]);
    }
    if (_queuesByTypes[COMPUTE_QUEUE] != nullptr)
    {
      return *static_cast<CommandQueueCompute*>(_queuesByTypes[COMPUTE_QUEUE]);
    }
    Abort("At least one of the graphic queue or the compute queue must exists");
  }

  inline const CommandQueueCompute& Device::primaryQueue() const noexcept
  {
    if(_queuesByTypes[GRAPHIC_QUEUE] != nullptr)
    {
      return *static_cast<CommandQueueCompute*>(_queuesByTypes[GRAPHIC_QUEUE]);
    }
    if (_queuesByTypes[COMPUTE_QUEUE] != nullptr)
    {
      return *static_cast<CommandQueueCompute*>(_queuesByTypes[COMPUTE_QUEUE]);
    }
    MT_ASSERT(false && "At least one of the graphic queue or the compute queue must exists");
  }

  inline const ExtFunctions& Device::extFunctions() const noexcept
  {
    return *_extFunctions;
  }
}