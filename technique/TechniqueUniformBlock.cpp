#include <technique/TechniqueUniformBlock.h>
#include <technique/Technique.h>
#include <vkr/queue/CommandProducerTransfer.h>

using namespace mt;

TechniqueUniformBlock::TechniqueUniformBlock(
                      const TechniqueConfiguration::UniformBuffer& description,
                      const Technique& technique) :
  _description(description),
  _technique(technique),
  _needUpdateGPUBuffer(false)
{
  MT_ASSERT(_description.size != 0);
  _cpuBuffer.resize(_description.size);

  _gpuBuffer = Ref(new DataBuffer(_technique.device(),
                                  _description.size,
                                  DataBuffer::UNIFORM_BUFFER));
}

void TechniqueUniformBlock::update(CommandProducerTransfer& commandProducer)
{
  if(_needUpdateGPUBuffer)
  {
    UniformMemoryPool::MemoryInfo writeInfo =
              commandProducer.uniformMemorySession().write( _cpuBuffer.data(),
                                                            _cpuBuffer.size());
    commandProducer.copyFromBufferToBuffer( *writeInfo.buffer,
                                            *_gpuBuffer,
                                            0,
                                            0,
                                            _cpuBuffer.size());
    _needUpdateGPUBuffer = false;
  }
}

void TechniqueUniformBlock::bindToDescriptorSet(DescriptorSet& set)
{
  set.attachUniformBuffer(*_gpuBuffer, _description.binding);
}
