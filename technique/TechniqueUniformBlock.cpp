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
                          CommandProducerTransfer& commandProducer,
                          const TechniqueVolatileContext* volatileContext) const
{
  if(_description.set == DescriptorSetType::STATIC)
  {
    UniformMemoryPool::MemoryInfo writeInfo =
              commandProducer.uniformMemorySession().write( _cpuBuffer.data(),
                                                            _cpuBuffer.size());

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
    //  Для начала определяем, откуда будем брать данные
    const char* srcData;
    if(volatileContext != nullptr)
    {
      srcData = (const char*)volatileContext->uniformData +
                                            _description.volatileContextOffset;
    }
    else
    {
      srcData = _cpuBuffer.data();
    }
    //  Записываем данные во временный GPU буфер. Для волатильного сета 
    //  долговременный буфер не нужен, поэтому сразу аттачим его к сету
    UniformMemoryPool::MemoryInfo writeInfo =
              commandProducer.uniformMemorySession().write( srcData,
                                                            _cpuBuffer.size());
    set.attachBuffer( *writeInfo.buffer,
                      _description.binding,
                      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      writeInfo.offset,
                      _description.size);
  }
}
