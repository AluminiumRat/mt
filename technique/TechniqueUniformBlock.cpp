#include <technique/TechniqueUniformBlock.h>
#include <technique/Technique.h>
#include <vkr/queue/CommandProducerTransfer.h>

using namespace mt;

TechniqueUniformBlock::TechniqueUniformBlock(
                      const TechniqueConfiguration::UniformBuffer& description,
                      const Technique& technique,
                      size_t& revisionCounter) :
  _description(description),
  _technique(technique),
  _revisionCounter(revisionCounter)
{
  MT_ASSERT(_description.size != 0);
  _cpuBuffer.resize(_description.size);
}

void TechniqueUniformBlock::bindToDescriptorSet(
                                      DescriptorSet& set,
                                      CommandProducerTransfer& commandProducer)
{
  UniformMemoryPool::MemoryInfo writeInfo =
              commandProducer.uniformMemorySession().write( _cpuBuffer.data(),
                                                            _cpuBuffer.size());

  if(_description.set == DescriptorSetType::STATIC)
  {
    // Для статик сета создаем новый полноценный буфер
    Ref<DataBuffer> gpuBuffer = Ref(
                                  new DataBuffer( _technique.device(),
                                                  _cpuBuffer.size(),
                                                  DataBuffer::UNIFORM_BUFFER));

    commandProducer.uniformBufferTransfer(*writeInfo.buffer,
                                          *gpuBuffer,
                                          0,
                                          0,
                                          _cpuBuffer.size());

    set.attachUniformBuffer(*gpuBuffer, _description.binding);
  }
  else
  {
    // Для волатильного сета не нужен долговременный буфер, используем временный
    set.attachBuffer( *writeInfo.buffer,
                      _description.binding,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      writeInfo.offset,
                      _description.size);
  }
}
