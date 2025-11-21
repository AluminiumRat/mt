#pragma once

#include <cstring>
#include <vector>

#include <technique/TechniqueConfiguration.h>
#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandProducerTransfer;
  class DescriptorSet;
  class Technique;

  //  Хранилище данных для одного униформ буфера. В нормальном режиме
  //    UniformVariable хранят свои значения в нем. Так же отвечает за
  //    создание и биндинг униформ буфера.
  //  Используется внутри техник (класс Technique), пользователь не должен
  //    взаимодействовать с этим классом
  class TechniqueUniformBlock
  {
  public:
    //  resvisionCounter - внешний счетчик ревизий. Позволяет отследить момент,
    //    когда дескриптер сет необходимо пересоздавать. Счетчик внешний и общий
    //    для всех блоков техники. Работает только для статик дескриптер сета,
    //    так как волатильный сет пересоздается на каждом кадре.
    //  resvisionCounter - внешний счетчик ревизий. Позволяет отследить момент,
    //    когда дескриптер сет необходимо пересоздавать. Счетчик внешний и общий
    //    для всех ресурсов техники. Работает только для статик дескриптер сета,
    //    так как волатильный сет пересоздается на каждом кадре.
    TechniqueUniformBlock(
                    const TechniqueConfiguration::UniformBuffer& description,
                    const Technique& technique,
                    size_t& revisionCounter);
    TechniqueUniformBlock(const TechniqueUniformBlock&) = delete;
    TechniqueUniformBlock& operator = (const TechniqueUniformBlock&) = delete;
    ~TechniqueUniformBlock() noexcept = default;

    inline const TechniqueConfiguration::UniformBuffer&
                                                  description() const noexcept;

    inline void setData(uint32_t offset,
                        uint32_t dataSize,
                        const void* srcData);
    inline const void* getData(uint32_t offset) const;

    void bindToDescriptorSet( DescriptorSet& set,
                              CommandProducerTransfer& commandProducer) const;

  private:
    const TechniqueConfiguration::UniformBuffer& _description;
    const Technique& _technique;
    size_t& _revisionCounter;

    std::vector<char> _cpuBuffer;
  };

  inline const TechniqueConfiguration::UniformBuffer&
                            TechniqueUniformBlock::description() const noexcept
  {
    return _description;
  }

  inline void TechniqueUniformBlock::setData( uint32_t offset,
                                              uint32_t dataSize,
                                              const void* srcData)
  {
    MT_ASSERT(offset + dataSize <= _description.size);
    memcpy(_cpuBuffer.data() + offset, srcData, dataSize);
    if(_description.set == DescriptorSetType::STATIC)
    {
      _revisionCounter++;
    }
  }

  inline const void* TechniqueUniformBlock::getData(uint32_t offset) const
  {
    MT_ASSERT(offset < _description.size);
    return _cpuBuffer.data() + offset;
  }
}