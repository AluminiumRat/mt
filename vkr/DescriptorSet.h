#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <util/Ref.h>
#include <util/RefCounter.h>
#include <vkr/image/ImagesAccessSet.h>

namespace mt
{
  class DataBuffer;
  class DescriptorPool;
  class Device;
  class ImageView;
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

    void attachUniformBuffer(const DataBuffer& buffer, uint32_t binding);
    void attachStorageBuffer(const DataBuffer& buffer, uint32_t binding);
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