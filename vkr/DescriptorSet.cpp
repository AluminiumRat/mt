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

void DescriptorSet::attachBuffer( const DataBuffer& buffer,
                                  uint32_t binding,
                                  VkDescriptorType descriptorType,
                                  VkDeviceSize offset,
                                  VkDeviceSize range)
{
  MT_ASSERT(!isFinalized());
  MT_ASSERT(offset + range < buffer.size())

  _resources.push_back(ConstRef(&buffer));

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer.handle();
  bufferInfo.offset = offset;
  bufferInfo.range = range;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = descriptorType;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}

void DescriptorSet::attachUniformBuffer(const DataBuffer& buffer,
                                        uint32_t binding)
{
  attachBuffer( buffer,
                binding,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                0,
                buffer.size());
}

void DescriptorSet::attachStorageBuffer(const DataBuffer& buffer,
                                        uint32_t binding)
{
  attachBuffer( buffer,
                binding,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                0,
                buffer.size());
}

void DescriptorSet::_addImageAccess(const ImageView& imageView,
                                    VkPipelineStageFlags stages,
                                    VkImageLayout layout,
                                    bool writeAccess)
{
  if (!imageView.image().isLayoutAutoControlEnabled()) return;
  ImageAccess imageAccess;
  imageAccess.slices[0] = imageView.slice();
  imageAccess.layouts[0] = layout;
  imageAccess.memoryAccess[0] = { .readStagesMask = stages,
                                  .readAccessMask = VK_ACCESS_SHADER_READ_BIT};
  if(writeAccess)
  {
    imageAccess.memoryAccess[0].writeStagesMask = stages;
    imageAccess.memoryAccess[0].writeAccessMask = VK_ACCESS_SHADER_READ_BIT;
  }
  imageAccess.slicesCount = 1;
  _imagesAccess.addImageAccess(imageView.image(), imageAccess);
}

void DescriptorSet::attachImage(const ImageView& imageView,
                                uint32_t binding,
                                VkDescriptorType descriptorType,
                                VkPipelineStageFlags stages,
                                bool writeAccess,
                                VkImageLayout layout)
{
  MT_ASSERT(!isFinalized());

  _resources.push_back(ConstRef(&imageView));
  _addImageAccess(imageView, stages, layout, writeAccess);

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageView = imageView.handle();
  imageInfo.imageLayout = layout;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.descriptorType = descriptorType;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets( _device.handle(),
                          1,
                          &descriptorWrite,
                          0,
                          nullptr);
}

void DescriptorSet::attachImages(
                                const std::vector<ConstRef<ImageView>>& images,
                                uint32_t binding,
                                VkDescriptorType descriptorType,
                                VkPipelineStageFlags stages,
                                bool writeAccess,
                                VkImageLayout layout)
{
  MT_ASSERT(!isFinalized());
  MT_ASSERT(!images.empty());

  // Выделяем место под VkDescriptorImageInfo для всех imageview
  size_t imagesInfoSize = sizeof(VkDescriptorImageInfo) * images.size();
  VkDescriptorImageInfo* imagesInfo =
                                (VkDescriptorImageInfo*)alloca(imagesInfoSize);

  for(uint32_t imageIndex = 0;
      imageIndex < uint32_t(images.size());
      imageIndex++)
  {
    const ImageView& imageView = *images[imageIndex];
    _resources.push_back(ConstRef(&imageView));
    _addImageAccess(imageView, stages, layout, writeAccess);

    // Заполняем инфу для дескриптер сета
    imagesInfo[imageIndex] = VkDescriptorImageInfo{};
    imagesInfo[imageIndex].imageView = imageView.handle();
    imagesInfo[imageIndex].imageLayout = layout;
  }

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = _handle;
  descriptorWrite.dstBinding = binding;
  descriptorWrite.descriptorType = descriptorType;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = uint32_t(images.size());
  descriptorWrite.pImageInfo = imagesInfo;

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
  attachImage(imageView,
              binding,
              VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
              stages,
              false,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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