#pragma once

#include <util/Ref.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  struct FrameBuildContext;
  class EnvironmentScene;

  //  Управляет набором общих ресурсов для шейдеров в ColorFrameBuilder-е
  class ColorFrameCommonSet
  {
  public:
    static constexpr uint32_t uniformBufferBinding = 0;

  public:
    //  Хелпер, который биндит сет в конструкторе и анбиндит в деструкторе
    class Bind
    {
    public:
      inline Bind(ColorFrameCommonSet& set,
                  CommandProducerGraphic& commandProducer);
      Bind(const Bind&) = delete;
      Bind& operator = (const Bind&) = delete;
      inline ~Bind() noexcept;

    private:
      ColorFrameCommonSet& _set;
      CommandProducerGraphic& _commandProducer;
    };

  public:
    explicit ColorFrameCommonSet(Device& device);
    ColorFrameCommonSet(const ColorFrameCommonSet&) = delete;
    ColorFrameCommonSet& operator = (const ColorFrameCommonSet&) = delete;
    ~ColorFrameCommonSet() = default;

    void update(CommandProducerGraphic& commandProducer,
                const FrameBuildContext& frameContext,
                const EnvironmentScene& environment);

  private:
    //  Вызывется из хэлпера Bind
    void bind(CommandProducerGraphic& commandProducer) const;
    void unbind(CommandProducerGraphic& commandProducer) const noexcept;

  private:
    void _createLayouts();
    void _updateuniformBuffer(CommandProducerGraphic& commandProducer,
                              const FrameBuildContext& frameContext,
                              const EnvironmentScene& environment);

  private:
    Device& _device;

    Ref<DescriptorSet> _descriptorSet;
    Ref<DataBuffer> _uniformBuffer;

    ConstRef<DescriptorSetLayout> _setLayout;
    Ref<PipelineLayout> _pipelineLayout;
  };

  inline ColorFrameCommonSet::Bind::Bind(
                                      ColorFrameCommonSet& set,
                                      CommandProducerGraphic& commandProducer) :
    _set(set),
    _commandProducer(commandProducer)
  {
    _set.bind(_commandProducer);
  }

  inline ColorFrameCommonSet::Bind::~Bind() noexcept
  {
    _set.unbind(_commandProducer);
  }
}