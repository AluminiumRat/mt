#pragma once

#include <technique/TechniqueConfiguration.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>

namespace mt
{
  class DescriptorSet;
  class Technique;

  //  Компонент класса Technique
  //  Буферы и текстуры.
  //  Объекты, которые подключается к пайплайну через дескриптеры и дескриптер
  //    сеты. Обычно используются как источник данных или место для результатов
  //    вычислений.
  //  Публичный интерфейс для использования снаружи Technique
  class TechniqueResource
  {
  public:
    //  resvisionCounter - внешний счетчик ревизий. Позволяет отследить момент,
    //    когда дескриптер сет необходимо пересоздавать. Счетчик внешний и общий
    //    для всех ресурсов техники. Работает только для статик дескриптер сета,
    //    так как волатильный сет пересоздается на каждом кадре.
    TechniqueResource(const char* name,
                      const Technique& technique,
                      size_t& revisionCounter,
                      const TechniqueConfiguration* configuration);
    TechniqueResource(const TechniqueResource&) = delete;
    TechniqueResource& operator = (const TechniqueResource&) = delete;
    ~TechniqueResource() noexcept = default;

    inline const std::string& name() const noexcept;

    inline const DataBuffer* buffer() const noexcept;
    inline void setBuffer(const DataBuffer* buffer);

    inline const ImageView* image(size_t arrayIndex = 0) const noexcept;
    inline void setImage(const ImageView* image, size_t arrayIndex = 0);

    inline const Sampler* sampler() const noexcept;
    inline void setSampler(const Sampler* sampler);

  protected:
    inline void _clear() noexcept;
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    std::string _name;
    const Technique& _technique;
    size_t& _resvisionCounter;
    const TechniqueConfiguration::Resource* _description;

    ConstRef<DataBuffer> _buffer;
    std::vector<ConstRef<ImageView>> _images;
    ConstRef<Sampler> _sampler;
  };

  //  Дополнение для внутреннего использования внутри техники
  class TechniqueResourceImpl : TechniqueResource
  {
  public:
    inline TechniqueResourceImpl( const char* name,
                                  const Technique& technique,
                                  size_t& revisionCounter,
                                  const TechniqueConfiguration* configuration);
    TechniqueResourceImpl(const TechniqueResourceImpl&) = delete;
    TechniqueResourceImpl& operator = (const TechniqueResourceImpl&) = delete;
    ~TechniqueResourceImpl() noexcept = default;

    inline void setConfiguration(const TechniqueConfiguration* configuration);
    void bindToDescriptorSet(DescriptorSet& set);
    inline const TechniqueConfiguration::Resource* description() const noexcept;

  private:
    void _bindSampler(DescriptorSet& set);
    void _bindBuffer(DescriptorSet& set);
    void _bindImage(DescriptorSet& set);
  };

  inline const std::string& TechniqueResource::name() const noexcept
  {
    return _name;
  }

  inline void TechniqueResource::_clear() noexcept
  {
    _buffer.reset();
    _images.clear();
    _sampler.reset();

    if( _description != nullptr &&
        _description->set == DescriptorSetType::STATIC)
    {
      _resvisionCounter++;
    }
  }

  inline const DataBuffer* TechniqueResource::buffer() const noexcept
  {
    return _buffer.get();
  }

  inline void TechniqueResource::setBuffer(const DataBuffer* buffer)
  {
    if(buffer == _buffer) return;
    _clear();
    _buffer = ConstRef(buffer);
  }

  inline const ImageView* TechniqueResource::image(
                                              size_t arrayIndex) const noexcept
  {
    if(_images.size() >= arrayIndex) return nullptr;
    return _images[arrayIndex].get();
  }

  inline void TechniqueResource::setImage(const ImageView* image,
                                          size_t arrayIndex)
  {
    if(_images.size() <= arrayIndex) _images.resize(arrayIndex + 1);

    if(image == _images[arrayIndex]) return;

    _buffer.reset();
    _sampler.reset();
    if( _description != nullptr &&
        _description->set == DescriptorSetType::STATIC)
    {
      _resvisionCounter++;
    }

    _images[arrayIndex] = ConstRef(image);
  }

  inline const Sampler* TechniqueResource::sampler() const noexcept
  {
    return _sampler.get();
  }

  inline void TechniqueResource::setSampler(const Sampler* sampler)
  {
    if(sampler == _sampler) return;
    _clear();
    _sampler = ConstRef(sampler);
  }

  inline TechniqueResourceImpl::TechniqueResourceImpl(
                                  const char* name,
                                  const Technique& technique,
                                  size_t& revisionCounter,
                                  const TechniqueConfiguration* configuration) :
    TechniqueResource(name, technique, revisionCounter, configuration)
  {
  }

  inline void TechniqueResourceImpl::setConfiguration(
                                    const TechniqueConfiguration* configuration)
  {
    _bindToConfiguration(configuration);
  }

  inline const TechniqueConfiguration::Resource*
                            TechniqueResourceImpl::description() const noexcept
  {
    return _description;
  }
}