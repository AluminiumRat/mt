#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>

namespace mt
{
  class Device;

  // Тонкая RAII обертка вокруг VkSampler
  // Настройки считывания текстуры из шейдера
  class Sampler : public RefCounter
  {
  public:
    Sampler(Device& device,
            VkFilter magFilter = VK_FILTER_LINEAR,
            VkFilter minFilter = VK_FILTER_LINEAR,
            VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            float mipLodBias = 0,
            bool anisotropyEnable = false,
            float maxAnisotropy = 4,
            bool compareEnable = false,
            VkCompareOp compareOp = VK_COMPARE_OP_GREATER,
            float minLod = 0,
            float maxLod = VK_LOD_CLAMP_NONE,
            VkBorderColor borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            bool unnormalizedCoordinates = false);
    Sampler(const Sampler&) = delete;
    Sampler& operator = (const Sampler&) = delete;
  protected:
    virtual ~Sampler();

  public:
    inline Device& device() const noexcept;
    inline VkSampler handle() const noexcept;

  private:
    Device& _device;
    VkSampler _handle;
  };

  inline Device& Sampler::device() const noexcept
  {
    return _device;
  }

  inline VkSampler Sampler::handle() const noexcept
  {
    return _handle;
  }
}