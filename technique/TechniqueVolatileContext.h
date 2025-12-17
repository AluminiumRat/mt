#pragma once

#include <cstdint>

#include <vkr/queue/CommandProducer.h>

namespace mt
{
  class CommandProducer;

  //  Структура, которая хранит временные значения для настройки техники во
  //    время обхода рендера.
  //  Позволяет донастроить технику и выставить волатильные переменные и сеты,
  //    не внося изменений в саму технику, реализуя константный рендер.
  class TechniqueVolatileContext
  {
  public:
    uint32_t* selectionsValues;       //  Таблица с индексами значений
                                      //  селекшенов
    void* uniformData;                //  Временный буфер для записи значений
                                      //  юниформ переменных
    Ref<DescriptorSet> descriptorSet; //  Дескриптер-сет для волатильных
                                      //  ресурсов

    uint32_t resourcesBinded;         //  Сколько ресурсов было прибинжено
                                      //  к волатильному дескриптер сету

  public:
    inline TechniqueVolatileContext(  CommandProducer& producer,
                                      const DescriptorSetLayout* setLayout,
                                      size_t selectionsNumber,
                                      size_t uniformsDataSize);
    TechniqueVolatileContext(const TechniqueVolatileContext&) = delete;
    TechniqueVolatileContext& operator = (
                                      const TechniqueVolatileContext&) = delete;
    inline TechniqueVolatileContext(TechniqueVolatileContext&& other) noexcept;
    inline TechniqueVolatileContext& operator = (
                                    TechniqueVolatileContext&& other) noexcept;

    inline ~TechniqueVolatileContext() noexcept;

  private:
    inline void _release() noexcept;

  private:
    CommandProducer* _producer;
  };

  inline TechniqueVolatileContext::TechniqueVolatileContext(
                                    CommandProducer& producer,
                                    const DescriptorSetLayout* setLayout,
                                    size_t selectionsNumber,
                                    size_t uniformsDataSize) :
    _producer(nullptr),
    selectionsValues(nullptr),
    uniformData(nullptr),
    descriptorSet(nullptr),
    resourcesBinded(0)
  {
    if(setLayout != nullptr)
    {
      descriptorSet = producer.descriptorPool().allocateSet(*setLayout);
    }

    if(selectionsNumber == 0 && uniformsDataSize == 0) return;

    _producer = &producer;
    size_t selectionsTableSize = selectionsNumber * sizeof(uint32_t);
    size_t tmpBufferSize = selectionsTableSize + uniformsDataSize;
    void* tmpBuffer =
                  producer.uniformMemorySession().lockTmpBuffer(tmpBufferSize);
    selectionsValues = (uint32_t*)tmpBuffer;
    uniformData = (char*)tmpBuffer + selectionsTableSize;
  }

  inline TechniqueVolatileContext::~TechniqueVolatileContext() noexcept
  {
    _release();
  }

  inline void TechniqueVolatileContext::_release() noexcept
  {
    if (_producer != nullptr)
    {
      _producer->uniformMemorySession().releaseTmpBuffer();
      _producer = nullptr;
    }
  }

  inline TechniqueVolatileContext::TechniqueVolatileContext(
                                    TechniqueVolatileContext&& other) noexcept :
    selectionsValues(other.selectionsValues),
    uniformData(other.uniformData),
    descriptorSet(other.descriptorSet),
    _producer(other._producer)
  {
    other._producer = nullptr;
  }

  inline TechniqueVolatileContext& TechniqueVolatileContext::operator = (
                                      TechniqueVolatileContext&& other) noexcept
  {
    _release();

    selectionsValues = other.selectionsValues;
    uniformData = other.uniformData;
    descriptorSet = other.descriptorSet;
    _producer = other._producer;
    other._producer = nullptr;
  }
}