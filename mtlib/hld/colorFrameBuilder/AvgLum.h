#pragma once

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

    inline void setSourceImage(ImageView& newImage) noexcept;

    //  Посчитать среднюю яркость изображения
    //  На момент вызова sourceImage должна либо быть в лэйауте
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, либо быть со включенным
    //    автоконтролем лэйаутов
    void update(CommandProducerCompute& commandProducer);

    //  Буфер, в котором после выполнения update будет находиться
    //  средняя яркость изображения
    inline const DataBuffer& resultBuffer() const noexcept;

  private:
    void _updateTechnique();
    void _updateBindings(CommandProducerCompute& commandProducer);
    void _average(CommandProducerCompute& commandProducer);

  private:
    Device& _device;

    Ref<ImageView> _sourceImage;
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

    //  Размер области, на которой будут работать компьют шейдеры
    glm::uvec2 _workAreaSize;
  };

  inline void AvgLum::setSourceImage(ImageView& newImage) noexcept
  {
    if(_sourceImage == &newImage) return;
    _sourceImage = &newImage;
    _sourceImageChanged = true;
  }

  inline const DataBuffer& AvgLum::resultBuffer() const noexcept
  {
    return *_resultBuffer;
  }
}