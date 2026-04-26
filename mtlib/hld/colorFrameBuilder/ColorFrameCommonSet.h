#pragma once

#include <hld/colorFrameBuilder/EnvironmentScene.h>
#include <util/Camera.h>
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
    static constexpr const char* setName = "ColorFrameCommonSet";
    static const VkDescriptorSetLayoutBinding bindings[13];

    static constexpr uint32_t uniformBufferBinding = 0;
    static constexpr uint32_t iblLutBinding = 1;
    static constexpr uint32_t iblLutSamplerBinding = 2;
    static constexpr uint32_t iblIrradianceMapBinding = 3;
    static constexpr uint32_t iblspecularMapBinding = 4;
    static constexpr uint32_t linearDepthHalfBufferBinding = 5;
    static constexpr uint32_t normalHalfBufferBinding = 6;
    static constexpr uint32_t roughnessHalfBufferBinding = 7;
    static constexpr uint32_t hiZBufferBinding = 8;
    static constexpr uint32_t reprojectionBufferBinding = 9;
    static constexpr uint32_t shadowBufferBinding = 10;
    static constexpr uint32_t commonLinearSamplerBinding = 11;
    static constexpr uint32_t commonNearestSamplerBinding = 12;

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
                const EnvironmentScene& environment,
                const ImageView& linearDepthHalfBuffer,
                const ImageView& normalHalfBuffer,
                const ImageView& roughnessHalfBuffer,
                const ImageView& hiZBuffer,
                const ImageView& reprojectionBuffer,
                const ImageView& shadowBuffer);

  private:
    //  Вызывется из хэлпера Bind
    void bind(CommandProducerGraphic& commandProducer) const;
    void unbind(CommandProducerGraphic& commandProducer) const noexcept;

  private:
    struct UniformCommonData
    {
      //  Информация о положении камеры на текущем кадре
      Camera::ShaderData cameraData;
      //  Информация о положении камеры на предыдущем кадре
      Camera::ShaderData prevCameraData;

      EnvironmentScene::UniformBufferData environment;

      alignas(16) glm::vec4 frameExtent;
      alignas(16) glm::uvec2 iFrameExtent;
      alignas(16) glm::vec4 halfExtent;
      alignas(16) glm::uvec2 iHalfExtent;
      alignas(16) glm::vec4 hiZExtent;
      alignas(16) glm::uvec2 iHiZExtent;
      alignas(4) uint32_t hiZMipCount;

      alignas(4) uint32_t frameIndex;
    };

  private:
    void _createLayouts();
    void _updateuniformBuffer(CommandProducerGraphic& commandProducer,
                              const FrameBuildContext& frameContext,
                              const EnvironmentScene& environment,
                              glm::uvec2 halfFrameSize,
                              glm::uvec2 hiZExtent,
                              uint32_t hiZMipCount);

  private:
    Device& _device;
    TextureManager& _textureManager;

    Ref<DescriptorSet> _descriptorSet;
    Ref<DataBuffer> _uniformBuffer;
    ConstRef<ImageView> _iblLut;
    Ref<Sampler> _iblLutSampler;
    //  Дефолтные сэмплеры общего назначения
    Ref<Sampler> _commonLinearSampler;
    Ref<Sampler> _commonNearestSampler;

    //  Информация о камере на текущем отрисовываемом кадре
    Camera::ShaderData _currentCameraData;
    //  Информация о камере на предыдущем кадре
    Camera::ShaderData _prevCameraData;

    uint32_t _frameIndex;

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