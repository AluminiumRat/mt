#include <vkr/DataBuffer.h>
#include <vkr/DescriptorPool.h>
#include <vkr/DescriptorSet.h>
#include <vkr/Device.h>

using namespace mt;

DescriptorSet::DescriptorSet( Device& device,
                              VkDescriptorSet handle,
                              const DescriptorPool* pool) :
  _device(device),
  _handle(handle)
{
  if(pool != nullptr)
  {
    _resources.push_back(ConstRef(pool));
  }
}

void DescriptorSet::attachUniformBuffer(const DataBuffer& buffer,
                                        uint32_t binding)
{
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
