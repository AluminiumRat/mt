#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  // Набор счетчиков дескриптеров разных типов. Используется для работы с
  // пулами дескриптеров.
  struct DescriptorCounter
  {
    uint32_t samplers = 0;
    uint32_t combinedImageSamplers = 0;
    uint32_t sampledImages = 0;
    uint32_t storageImages = 0;
    uint32_t uniformTexelBuffers = 0;
    uint32_t storageTexelBuffers = 0;
    uint32_t uniformBuffers = 0;
    uint32_t storageBuffers = 0;
    uint32_t inputAttachments = 0;

    inline void reset() noexcept;

    inline bool contains(const DescriptorCounter& other) const  noexcept;

    void add(VkDescriptorType descriptorType, uint32_t value = 1)  noexcept;
    inline void add(const DescriptorCounter& other)  noexcept;

    void reduce(VkDescriptorType descriptorType, uint32_t value = 1)  noexcept;
    inline void reduce(const DescriptorCounter& other)  noexcept;

    std::vector<VkDescriptorPoolSize> makeSizeInfo() const;

    static DescriptorCounter createFrom(
              std::span<const VkDescriptorSetLayoutBinding> bindings) noexcept;
  };

  inline void DescriptorCounter::reset() noexcept
  {
    samplers = 0;
    combinedImageSamplers = 0;
    sampledImages = 0;
    storageImages = 0;
    uniformTexelBuffers = 0;
    storageTexelBuffers = 0;
    uniformBuffers = 0;
    storageBuffers = 0;
    inputAttachments = 0;
  }

  inline bool DescriptorCounter::contains(
                                  const DescriptorCounter& other) const noexcept
  {
    return  samplers >= other.samplers &&
            combinedImageSamplers >= combinedImageSamplers &&
            sampledImages >= other.sampledImages &&
            storageImages >= other.storageImages &&
            uniformTexelBuffers >= other.uniformTexelBuffers &&
            storageTexelBuffers >= other.storageTexelBuffers &&
            uniformBuffers >= other.uniformBuffers &&
            storageBuffers >= other.storageBuffers &&
            inputAttachments >= other.inputAttachments;
  }

  inline void DescriptorCounter::add(const DescriptorCounter& other) noexcept
  {
    samplers += other.samplers;
    combinedImageSamplers += other.combinedImageSamplers;
    sampledImages += other.sampledImages;
    storageImages += other.storageImages;
    uniformTexelBuffers += other.uniformTexelBuffers;
    storageTexelBuffers += other.storageTexelBuffers;
    uniformBuffers += other.uniformBuffers;
    storageBuffers += other.storageBuffers;
    inputAttachments += other.inputAttachments;
  }

  inline void DescriptorCounter::reduce(const DescriptorCounter& other) noexcept
  {
    samplers -= other.samplers;
    combinedImageSamplers -= other.combinedImageSamplers;
    sampledImages -= other.sampledImages;
    storageImages -= other.storageImages;
    uniformTexelBuffers -= other.uniformTexelBuffers;
    storageTexelBuffers -= other.storageTexelBuffers;
    uniformBuffers -= other.uniformBuffers;
    storageBuffers -= other.storageBuffers;
    inputAttachments -= other.inputAttachments;
  }
}