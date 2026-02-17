#pragma once

#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/pipeline/PipelineLayout.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>

namespace mt
{
  class CommandProducerGraphic;
  class Device;
  class EnvironmentScene;
  struct FrameBuildContext;
  class TextureManager;


  //  Управляет набором общих ресурсов для шейдеров в ColorFrameBuilder-е
  class ColorFrameCommonSet
  {
  public:
    static constexpr uint32_t uniformBufferBinding = 0;
    static constexpr uint32_t iblLutBinding = 1;
    static constexpr uint32_t iblLutSamplerBinding = 2;
    static constexpr uint32_t iblIrradianceMapBinding = 3;
    static constexpr uint32_t iblspecularMapBinding = 4;
    static constexpr uint32_t commonLinearSamplerBinding = 5;

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
    ColorFrameCommonSet(Device& device, TextureManager& textureManager);
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
    TextureManager& _textureManager;

    Ref<DescriptorSet> _descriptorSet;
    Ref<DataBuffer> _uniformBuffer;
    ConstRef<ImageView> _iblLut;
    Ref<Sampler> _iblLutSampler;
    //  Дефолтный сэмплер общего назначения
    Ref<Sampler> _commonLinearSampler;

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