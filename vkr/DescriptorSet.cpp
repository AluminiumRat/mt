#include <util/Assert.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/DescriptorPool.h>
#include <vkr/DescriptorSet.h>
#include <vkr/Device.h>
#include <vkr/Sampler.h>

using namespace mt;

DescriptorSet::DescriptorSet( Device& device,
                              VkDescriptorSet handle,
                              const DescriptorPool* pool) :
  _device(device),
  _handle(handle),
  _finalized(false)
{
  if(pool != nullptr)
  {
    _resources.push_back(ConstRef(pool));
  }
}

void DescriptorSet::attachUniformBuffer(const DataBuffer& buffer,
                                        uint32_t binding)
{
  MT_ASSERT(!isFinalized());

  _resources.push_back(ConstRef(&buffer));

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer.handle();
  bufferInfo.offset = 0;
  bufferInfo.range = buffer.size();

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}

void DescriptorSet::attachStorageBuffer(const DataBuffer& buffer,
                                        uint32_t binding)
{
  MT_ASSERT(!isFinalized());

  _resources.push_back(ConstRef(&buffer));

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer.handle();
  bufferInfo.offset = 0;
  bufferInfo.range = buffer.size();

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}

void DescriptorSet::attachSampledImage( const ImageView& imageView,
                                        uint32_t binding,
                                        VkPipelineStageFlags stages)
{
  MT_ASSERT(!isFinalized());

  _resources.push_back(ConstRef(&imageView));

  const Image& image = imageView.image();
  if(image.isLayoutAutoControlEnabled())
  {
    ImageAccess imageAccess;
    imageAccess.slices[0] = imageView.slice();
    imageAccess.layouts[0] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageAccess.memoryAccess[0] = {
                                  .readStagesMask = stages,
                                  .readAccessMask = VK_ACCESS_SHADER_READ_BIT};
    imageAccess.slicesCount = 1;
    _imagesAccess.addImageAccess(image, imageAccess);
  }

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageView = imageView.handle();
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}

void DescriptorSet::attachSampler(const Sampler& sampler,uint32_t binding)
{
  MT_ASSERT(!isFinalized());

  _resources.push_back(ConstRef(&sampler));

  VkDescriptorImageInfo imageInfo{};
  imageInfo.sampler = sampler.handle();

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}