#pragma once

#include <chrono>
#include <optional>

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/DataBuffer.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  class AvgLum
  {
  public:
    explicit AvgLum(Device& device);
    AvgLum(const AvgLum&) = delete;
    AvgLum& operator = (const AvgLum&) = delete;
    ~AvgLum() noexcept = default;

    inline void setSourceImage(const ImageView& newImage) noexcept;

    //  Посчитать среднюю яркость изображения
    //  На момент вызова sourceImage должна либо быть в лэйауте
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, либо быть со включенным
    //    автоконтролем лэйаутов
    void update(CommandProducerCompute& commandProducer);

    //  Буфер, в котором после выполнения update будет находиться
    //  средняя яркость изображения
    inline const DataBuffer& resultBuffer() const noexcept;

    inline float accommodationSpeed() const noexcept;
    inline void setAccommodationSpeed(float newValue) noexcept;

  private:
    void _updateTechnique();
    void _updateBindings(CommandProducerCompute& commandProducer);
    void _updateMixFactor();
    void _average(CommandProducerCompute& commandProducer);

  private:
    Device& _device;

    ConstRef<ImageView> _sourceImage;
    bool _sourceImageChanged;

    Ref<Image> _intermediateImage;
    Ref<DataBuffer> _resultBuffer;

    Ref<TechniqueConfigurator> _techniqueConfigurator;
    Technique _technique;
    TechniquePass& _horizontalPass;
    TechniquePass& _verticalPass;
    ResourceBinding& _sourceImageBinding;
    ResourceBinding& _intermediateImageBinding;
    ResourceBinding& _resultBufferBinding;
    UniformVariable& _invSourceSizeUniform;
    UniformVariable& _areaSizeUniform;
    UniformVariable& _pixelRateUniform;
    UniformVariable& _mixFactorUniform;

    //  Размер области, на которой будут работать компьют шейдеры
    glm::uvec2 _workAreaSize;

    std::optional<std::chrono::system_clock::time_point> _lastUpadateTime;
    float _accommodationSpeed;
  };

  inline void AvgLum::setSourceImage(const ImageView& newImage) noexcept
  {
    if(_sourceImage == &newImage) return;
    _sourceImage = &newImage;
    _sourceImageChanged = true;
  }

  inline const DataBuffer& AvgLum::resultBuffer() const noexcept
  {
    return *_resultBuffer;
  }

  inline float AvgLum::accommodationSpeed() const noexcept
  {
    return _accommodationSpeed;
  }

  inline void AvgLum::setAccommodationSpeed(float newValue) noexcept
  {
    _accommodationSpeed = newValue;
  }
}