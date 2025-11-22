#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include <technique/TechniqueConfiguration.h>
#include <util/Assert.h>
#include <util/Ref.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>

namespace mt
{
  class DescriptorSet;
  class Technique;
  class TechniqueVolatileContext;

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
    inline const ImageView* image(size_t arrayIndex = 0) const noexcept;
    inline const Sampler* sampler() const noexcept;

    //  Установить буфер, сохранив указатель на него в сам TechniqueResource
    //  Ссылка сохраняется в технике между циклами рендера
    inline void setBuffer(const DataBuffer* buffer);
    inline void setBuffer(const Ref<DataBuffer>& buffer);
    inline void setBuffer(const ConstRef<DataBuffer>& buffer);
    //  Установить буфер в контекст отрисовки. Ссылка в технике не сохраняется.
    void setBuffer( TechniqueVolatileContext& context,
                    const DataBuffer& buffer) const;
    inline void setBuffer(TechniqueVolatileContext& context,
                          const Ref<DataBuffer>& buffer) const;
    inline void setBuffer(TechniqueVolatileContext& context,
                          const ConstRef<DataBuffer>& buffer) const;

    //  Установить Image или массив Image-ей, сохранив указатель на них в сам
    //    TechniqueResource
    //  Ссылка сохраняется в технике между циклами рендера
    inline void setImage(const ImageView* image, size_t arrayIndex = 0);
    inline void setImage(const Ref<ImageView>& image, size_t arrayIndex = 0);
    inline void setImage( const ConstRef<ImageView>& image,
                          size_t arrayIndex = 0);
    inline void setImages(std::span<const ImageView*> images);
    inline void setImages(std::span<const Ref<ImageView>> images);
    inline void setImages(std::span<const ConstRef<ImageView>> images);
    //  Установить Image или массив Image-ей в контекст отрисовки
    //  Ссылка на ресурсы в технике не сохраняется.
    void setImage(TechniqueVolatileContext& context,
                  const ImageView& image) const;
    inline void setImage( TechniqueVolatileContext& context,
                          const Ref<ImageView>& image) const;
    inline void setImage( TechniqueVolatileContext& context,
                          const ConstRef<ImageView>& image) const;
    void setImages( TechniqueVolatileContext& context,
                    std::span<const ConstRef<ImageView>> images) const;

    //  Установить сэмплер, сохранив указатель на него в сам TechniqueResource
    //  Ссылка сохраняется в технике между циклами рендера
    inline void setSampler(const Sampler* sampler);
    inline void setSampler(const Ref<Sampler>& sampler);
    inline void setSampler(const ConstRef<Sampler>& sampler);
    //  Установить сэмплер в контекст отрисовки.
    //  Ссылка в технике не сохраняется.
    void setSampler(TechniqueVolatileContext& context,
                    const Sampler& sampler) const;
    inline void setSampler( TechniqueVolatileContext& context,
                            const Ref<Sampler>& sampler) const;
    inline void setSampler( TechniqueVolatileContext& context,
                            const ConstRef<Sampler>& sampler) const;

  protected:
    inline void _clear() noexcept;
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    std::string _name;
    const Technique& _technique;
    size_t& _revisionCounter;
    const TechniqueConfiguration::Resource* _description;

    ConstRef<DataBuffer> _buffer;
    std::vector<ConstRef<ImageView>> _images;
    ConstRef<Sampler> _sampler;
  };

  //  Дополнение для внутреннего использования внутри техники
  class TechniqueResourceImpl : public TechniqueResource
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
    void bindToDescriptorSet(DescriptorSet& set) const;
    inline const TechniqueConfiguration::Resource* description() const noexcept;

  private:
    void _bindSampler(DescriptorSet& set) const;
    void _bindBuffer(DescriptorSet& set) const;
    void _bindImage(DescriptorSet& set) const;
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
      _revisionCounter++;
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

  inline void TechniqueResource::setBuffer(const Ref<DataBuffer>& buffer)
  {
    setBuffer(buffer.get());
  }

  inline void TechniqueResource::setBuffer(const ConstRef<DataBuffer>& buffer)
  {
    setBuffer(buffer.get());
  }

  inline void TechniqueResource::setBuffer(
                                          TechniqueVolatileContext& context,
                                          const Ref<DataBuffer>& buffer) const
  {
    MT_ASSERT(buffer != nullptr);
    setBuffer(context, *buffer);
  }

  inline void TechniqueResource::setBuffer(
                                      TechniqueVolatileContext& context,
                                      const ConstRef<DataBuffer>& buffer) const
  {
    MT_ASSERT(buffer != nullptr);
    setBuffer(context, *buffer);
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
      _revisionCounter++;
    }

    _images[arrayIndex] = ConstRef(image);
  }

  inline void TechniqueResource::setImage(const Ref<ImageView>& image,
                                          size_t arrayIndex)
  {
    setImage(image.get(), arrayIndex);
  }

  inline void TechniqueResource::setImage(const ConstRef<ImageView>& image,
                                          size_t arrayIndex)
  {
    setImage(image.get(), arrayIndex);
  }

  inline void TechniqueResource::setImages(std::span<const ImageView*> images)
  {
    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    _clear();
    _images = std::vector<ConstRef<ImageView>>(images.begin(), images.end());
  }

  inline void TechniqueResource::setImages(
                                        std::span<const Ref<ImageView>> images)
  {
    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    _clear();
    _images = std::vector<ConstRef<ImageView>>(images.begin(), images.end());
  }

  inline void TechniqueResource::setImages(
                                    std::span<const ConstRef<ImageView>> images)
  {
    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    _clear();
    _images = std::vector(images.begin(), images.end());
  }

  inline void TechniqueResource::setImage(TechniqueVolatileContext& context,
                                          const Ref<ImageView>& image) const
  {
    MT_ASSERT(image != nullptr);
    setImage(context, *image);
  }

  inline void TechniqueResource::setImage(
                                        TechniqueVolatileContext& context,
                                        const ConstRef<ImageView>& image) const
  {
    MT_ASSERT(image != nullptr);
    setImage(context, *image);
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

  inline void TechniqueResource::setSampler(const Ref<Sampler>& sampler)
  {
    setSampler(sampler.get());
  }

  inline void TechniqueResource::setSampler(const ConstRef<Sampler>& sampler)
  {
    setSampler(sampler.get());
  }

  inline void TechniqueResource::setSampler(TechniqueVolatileContext& context,
                                            const Ref<Sampler>& sampler) const
  {
    MT_ASSERT(sampler != nullptr);
    setSampler(context, *sampler);
  }

  inline void TechniqueResource::setSampler(
                                        TechniqueVolatileContext& context,
                                        const ConstRef<Sampler>& sampler) const
  {
    MT_ASSERT(sampler != nullptr);
    setSampler(context, *sampler);
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