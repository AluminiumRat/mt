#pragma once

#include <technique/TechniqueConfigurator.h>
#include <technique/Technique.h>
#include <util/Ref.h>
#include <vkr/image/Image.h>

namespace mt
{
  class CommandProducerCompute;
  class Device;

  //  Создает размытую текстуру для наложения блума поверх HDR изображения
  class Bloom
  {
  public:
    //  avgLumBuffer - буффер, в первых 4 байтах которого лежит float со средней
    //    яркостью изображения. Можно получить из класса AvgLum
    Bloom(Device& device, const DataBuffer& avgLumBuffer);
    Bloom(const Bloom&) = delete;
    Bloom& operator = (const Bloom&) = delete;
    ~Bloom() noexcept = default;

    inline float treshold() const noexcept;
    inline void setTreshold(float newValue);

    inline float intensity() const noexcept;
    inline void setIntensity(float newValue);

    inline void setSourceImage(ImageView& newImage) noexcept;

    //  Построить блюренную текстуру по исходному hdr буферу
    //  На момент вызова sourceImage должна либо быть в лэйауте
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, либо быть со включенным
    //    автоконтролем лэйаутов
    void update(CommandProducerCompute& commandProducer);

    //  Разблюренный image гоиовый к наложению на HDR буфер
    //  Возвращает nullptr, если update ещё не был вызван, либо последний
    //    вызов закончисля исключением
    //  Возвращает Image с выключенным автоконтролем и в лэйауте
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    inline const Image* bloomImage() const noexcept;

    void makeGui();

  private:
    void _updateTechnique();
    void _updateBindings();
    void _blur(CommandProducerCompute& commandProducer);

  private:
    Device& _device;

    float _treshold;
    float _intensity;

    Ref<ImageView> _sourceImage;
    bool _sourceImageChanged;

    Ref<Image> _bloomImage;
    glm::uvec2 _bloomImageSize;

    Ref<TechniqueConfigurator> _techniqueConfigurator;
    Technique _blurTechnique;
    TechniquePass& _horizontalPass;
    TechniquePass& _verticalPass;
    ResourceBinding& _sourceImageBinding;
    ResourceBinding& _targetImageBinding;
    ResourceBinding& _avgLumBinding;
    UniformVariable& _invSourceSizeUniform;
    UniformVariable& _targetSizeUniform;
    UniformVariable& _tresholdUniform;
    UniformVariable& _intensityUniform;
  };

  inline void Bloom::setSourceImage(ImageView& newImage) noexcept
  {
    if(_sourceImage == &newImage) return;
    _sourceImage = &newImage;
    _sourceImageChanged = true;
  }

  inline const Image* Bloom::bloomImage() const noexcept
  {
    return _bloomImage.get();
  }

  inline float Bloom::treshold() const noexcept
  {
    return _treshold;
  }

  inline void Bloom::setTreshold(float newValue)
  {
    if(_treshold == newValue) return;
    _treshold = newValue;
    _tresholdUniform.setValue(_treshold);
  }

  inline float Bloom::intensity() const noexcept
  {
    return _intensity;
  }

  inline void Bloom::setIntensity(float newValue)
  {
    if(_intensity == newValue) return;
    _intensity = newValue;
    _intensityUniform.setValue(newValue);
  }
}