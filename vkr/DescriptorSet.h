#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/image/ImagesAccessSet.h>
#include <vkr/image/ImageView.h>

namespace mt
{
  class DataBuffer;
  class DescriptorPool;
  class Device;
  class Sampler;

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

    inline const ImagesAccessSet& imagesAccess() const noexcept;

    // Подключить буфер, общий способ
    void attachBuffer(const DataBuffer& buffer,
                      uint32_t binding,
                      VkDescriptorType descriptorType,
                      VkDeviceSize offset,
                      VkDeviceSize range);
    // Подключить буфер, упрощенный вариант
    void attachUniformBuffer(const DataBuffer& buffer, uint32_t binding);
    // Подключить буфер, упрощенный вариант
    void attachStorageBuffer(const DataBuffer& buffer, uint32_t binding);

    // Подключить один ImageView. Общий метод.
    void attachImage( const ImageView& imageView,
                      uint32_t binding,
                      VkDescriptorType descriptorType,
                      VkPipelineStageFlags stages,
                      bool writeAccess,
                      VkImageLayout layout);
    // Подключить массив ImageView. Общий метод.
    void attachImages(std::span<const ConstRef<ImageView>> images,
                      uint32_t binding,
                      VkDescriptorType descriptorType,
                      VkPipelineStageFlags stages,
                      bool writeAccess,
                      VkImageLayout layout);
    // Подключить ImageView на чтение через сэмплер. Упрощенный вариант.
    void attachSampledImage(const ImageView& imageView,
                            uint32_t binding,
                            VkPipelineStageFlags stages);

    void attachSampler(const Sampler& sampler, uint32_t binding);

    //  В финализированные сеты нельзя добавлять новые ресурсы, но можно
    //    биндить в комманд продюсерах. Это защита от изменения сета во время
    //    использования.
    inline void finalize() noexcept;
    inline bool isFinalized() const noexcept;

  private:
    void _addImageAccess( const ImageView& imageView,
                          VkPipelineStageFlags stages,
                          VkImageLayout layout,
                          bool writeAccess);

  private:
    Device& _device;
    VkDescriptorSet _handle;

    std::vector<ConstRef<RefCounter>> _resources;
    ImagesAccessSet _imagesAccess;

    bool _finalized;
  };

  inline Device& DescriptorSet::device() const noexcept
  {
    return _device;
  }

  inline VkDescriptorSet DescriptorSet::handle() const noexcept
  {
    return _handle;
  }

  inline const ImagesAccessSet& DescriptorSet::imagesAccess() const noexcept
  {
    return _imagesAccess;
  }

  inline void DescriptorSet::finalize() noexcept
  {
    _finalized = true;
  }

  inline bool DescriptorSet::isFinalized() const noexcept
  {
    return _finalized;
  }
}