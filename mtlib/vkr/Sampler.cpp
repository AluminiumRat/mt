#include <util/Abort.h>
#include <vkr/Device.h>
#include <vkr/Sampler.h>

using namespace mt;

Sampler::Sampler( Device& device,
                  VkFilter magFilter,
                  VkFilter minFilter,
                  VkSamplerMipmapMode mipmapMode,
                  VkSamplerAddressMode addressModeU,
                  VkSamplerAddressMode addressModeV,
                  VkSamplerAddressMode addressModeW,
                  float mipLodBias,
                  bool anisotropyEnable,
                  float maxAnisotropy,
                  bool compareEnable,
                  VkCompareOp compareOp,
                  float minLod,
                  float maxLod,
                  VkBorderColor borderColor,
                  bool unnormalizedCoordinates) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = magFilter;
  samplerInfo.minFilter = minFilter;
  samplerInfo.mipmapMode = mipmapMode;
  samplerInfo.addressModeU = addressModeU;
  samplerInfo.addressModeV = addressModeV;
  samplerInfo.addressModeW = addressModeW;
  samplerInfo.mipLodBias = mipLodBias;
  samplerInfo.anisotropyEnable = anisotropyEnable;
  samplerInfo.maxAnisotropy = maxAnisotropy;
  samplerInfo.compareEnable = compareEnable;
  samplerInfo.compareOp = compareOp;
  samplerInfo.minLod = minLod;
  samplerInfo.maxLod = maxLod;
  samplerInfo.borderColor = borderColor;
  samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates;

  if(vkCreateSampler( device.handle(),
                      &samplerInfo,
                      nullptr,
                      &_handle) != VK_SUCCESS)
  {
    Abort("Unable to create sampler");
  }
}

Sampler::Sampler(Device& device, const SamplerDescription& description) :
  _device(device),
  _handle(VK_NULL_HANDLE)
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = description.magFilter;
  samplerInfo.minFilter = description.minFilter;
  samplerInfo.mipmapMode = description.mipmapMode;
  samplerInfo.addressModeU = description.addressModeU;
  samplerInfo.addressModeV = description.addressModeV;
  samplerInfo.addressModeW = description.addressModeW;
  samplerInfo.mipLodBias = description.mipLodBias;
  samplerInfo.anisotropyEnable = description.anisotropyEnable;
  samplerInfo.maxAnisotropy = description.maxAnisotropy;
  samplerInfo.compareEnable = description.compareEnable;
  samplerInfo.compareOp = description.compareOp;
  samplerInfo.minLod = description.minLod;
  samplerInfo.maxLod = description.maxLod;
  samplerInfo.borderColor = description.borderColor;
  samplerInfo.unnormalizedCoordinates = description.unnormalizedCoordinates;

  if(vkCreateSampler( device.handle(),
                      &samplerInfo,
                      nullptr,
                      &_handle) != VK_SUCCESS)
  {
    Abort("Unable to create sampler");
  }
}

Sampler::~Sampler()
{
  vkDestroySampler(_device.handle(), _handle, nullptr);
}