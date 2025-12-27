#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include <technique/TechniqueConfiguration.h>
#include <technique/TechniqueResource.h>
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
  //  Способ подключить буферы, текстуры и сэмплеры.
  //  Объекты, которые подключается к пайплайну через дескриптеры и дескриптер
  //    сеты. Обычно используются как источник данных или место для результатов
  //    вычислений.
  //  Позволяет подключать как статические источники данных (DataBuffer,
  //    ImageView и Sampler), так и динамические (TechniqueResource)
  //  Публичный интерфейс для использования снаружи Technique
  class ResourceBinding : public TechniqueResource::Observer
  {
  public:
    //  resvisionCounter - внешний счетчик ревизий. Позволяет отследить момент,
    //    когда дескриптер сет необходимо пересоздавать. Счетчик внешний и общий
    //    для всех ресурсов техники. Работает только для статик дескриптер сета,
    //    так как волатильный сет пересоздается на каждом кадре.
    ResourceBinding(const char* name,
                    const Technique& technique,
                    size_t& revisionCounter,
                    const TechniqueConfiguration* configuration);
    ResourceBinding(const ResourceBinding&) = delete;
    ResourceBinding& operator = (const ResourceBinding&) = delete;
    ~ResourceBinding() noexcept = default;

    inline const std::string& name() const noexcept;

    inline const DataBuffer* buffer() const noexcept;
    inline const ImageView* image(size_t arrayIndex = 0) const noexcept;
    inline const Sampler* sampler() const noexcept;
    //  К ресурсу не прибинжен ни image, ни буффер, ни сэмплер
    inline bool empty() const noexcept;

    inline const TechniqueResource* resource() const noexcept;

    //  Установить буфер, сохранив указатель на него в сам ResourceBinding
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
    //    ResourceBinding
    //  Ссылка сохраняется в технике между циклами рендера
    inline void setImage(const ImageView* image);
    inline void setImage(const Ref<ImageView>& image);
    inline void setImage( const ConstRef<ImageView>& image);
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

    //  Установить сэмплер, сохранив указатель на него в сам ResourceBinding
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

    void setResource(const TechniqueResource* theResource);
    inline void setResource(const Ref<TechniqueResource>& theResource);
    inline void setResource(const ConstRef<TechniqueResource>& theResource);

    // Сбросить все подключенные ресурсы
    inline void clear() noexcept;

  protected:
    virtual void onResourceUpdated() override;

  protected:
    void _bindToConfiguration(const TechniqueConfiguration* configuration);

  protected:
    std::string _name;
    const Technique& _technique;
    size_t& _revisionCounter;
    const TechniqueConfiguration::Resource* _description;

    ConstRef<DataBuffer> _buffer;
    std::vector<ConstRef<ImageView>> _images;
    ConstRef<Sampler> _sampler;

    //  К ресурсу не прибинжен ни image, ни буффер, ни сэмплер
    bool _empty;

    //  Текущий image, буффер или сэмплер является значением по умолчанию
    bool _defaultValue;
  };

  //  Дополнение для внутреннего использования внутри техники
  class ResourceBindingImpl : public ResourceBinding
  {
  public:
    inline ResourceBindingImpl( const char* name,
                                const Technique& technique,
                                size_t& revisionCounter,
                                const TechniqueConfiguration* configuration);
    ResourceBindingImpl(const ResourceBindingImpl&) = delete;
    ResourceBindingImpl& operator = (const ResourceBindingImpl&) = delete;
    ~ResourceBindingImpl() noexcept = default;

    inline void setConfiguration(const TechniqueConfiguration* configuration);
    void bindToDescriptorSet(DescriptorSet& set) const;
    inline const TechniqueConfiguration::Resource* description() const noexcept;

    inline bool isDefault() const noexcept;
    inline void setSamplerAsDefault(const Sampler* sampler);

  private:
    void _bindSampler(DescriptorSet& set) const;
    void _bindBuffer(DescriptorSet& set) const;
    void _bindImage(DescriptorSet& set) const;
  };

  inline const std::string& ResourceBinding::name() const noexcept
  {
    return _name;
  }

  inline void ResourceBinding::clear() noexcept
  {
    _buffer.reset();
    _images.clear();
    _sampler.reset();
    detach();

    _empty = true;

    if( _description != nullptr &&
        _description->set == DescriptorSetType::STATIC)
    {
      _revisionCounter++;
    }
  }

  inline const DataBuffer* ResourceBinding::buffer() const noexcept
  {
    return _buffer.get();
  }

  inline void ResourceBinding::setBuffer(const DataBuffer* buffer)
  {
    _defaultValue = false;
    if(buffer == _buffer) return;
    clear();
    _buffer = ConstRef(buffer);
    if(_buffer != nullptr) _empty = false;
  }

  inline void ResourceBinding::setBuffer(const Ref<DataBuffer>& buffer)
  {
    setBuffer(buffer.get());
  }

  inline void ResourceBinding::setBuffer(const ConstRef<DataBuffer>& buffer)
  {
    setBuffer(buffer.get());
  }

  inline void ResourceBinding::setBuffer( TechniqueVolatileContext& context,
                                          const Ref<DataBuffer>& buffer) const
  {
    MT_ASSERT(buffer != nullptr);
    setBuffer(context, *buffer);
  }

  inline void ResourceBinding::setBuffer(
                                      TechniqueVolatileContext& context,
                                      const ConstRef<DataBuffer>& buffer) const
  {
    MT_ASSERT(buffer != nullptr);
    setBuffer(context, *buffer);
  }

  inline const ImageView* ResourceBinding::image(
                                              size_t arrayIndex) const noexcept
  {
    if(_images.size() <= arrayIndex) return nullptr;
    return _images[arrayIndex].get();
  }

  inline void ResourceBinding::setImage(const ImageView* image)
  {
    if(_images.size() == 1 && image == _images[0]) return;
    if(image == nullptr) clear();
    else setImages(std::span(&image, 1));
  }

  inline void ResourceBinding::setImage(const Ref<ImageView>& image)
  {
    setImage(image.get());
  }

  inline void ResourceBinding::setImage(const ConstRef<ImageView>& image)
  {
    setImage(image.get());
  }

  inline void ResourceBinding::setImages(std::span<const ImageView*> images)
  {
    _defaultValue = false;

    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    clear();
    _images = std::vector<ConstRef<ImageView>>(images.begin(), images.end());
    if(!_images.empty()) _empty = false;
  }

  inline void ResourceBinding::setImages(
                                        std::span<const Ref<ImageView>> images)
  {
    _defaultValue = false;

    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    clear();
    _images = std::vector<ConstRef<ImageView>>(images.begin(), images.end());
    if(!_images.empty()) _empty = false;
  }

  inline void ResourceBinding::setImages(
                                    std::span<const ConstRef<ImageView>> images)
  {
    _defaultValue = false;

    if( _images.size() == images.size() &&
        std::equal(_images.begin(), _images.end(), images.begin()))
    {
      return;
    }
    clear();
    _images = std::vector(images.begin(), images.end());
    if(!_images.empty()) _empty = false;
  }

  inline void ResourceBinding::setImage(TechniqueVolatileContext& context,
                                        const Ref<ImageView>& image) const
  {
    MT_ASSERT(image != nullptr);
    setImage(context, *image);
  }

  inline void ResourceBinding::setImage(TechniqueVolatileContext& context,
                                        const ConstRef<ImageView>& image) const
  {
    MT_ASSERT(image != nullptr);
    setImage(context, *image);
  }

  inline const Sampler* ResourceBinding::sampler() const noexcept
  {
    return _sampler.get();
  }

  inline void ResourceBinding::setSampler(const Sampler* sampler)
  {
    _defaultValue = false;
    if(sampler == _sampler) return;
    clear();
    _sampler = ConstRef(sampler);
    if(_sampler != nullptr) _empty = false;
  }

  inline void ResourceBinding::setSampler(const Ref<Sampler>& sampler)
  {
    setSampler(sampler.get());
  }

  inline void ResourceBinding::setSampler(const ConstRef<Sampler>& sampler)
  {
    setSampler(sampler.get());
  }

  inline void ResourceBinding::setSampler(TechniqueVolatileContext& context,
                                          const Ref<Sampler>& sampler) const
  {
    MT_ASSERT(sampler != nullptr);
    setSampler(context, *sampler);
  }

  inline void ResourceBinding::setSampler(
                                        TechniqueVolatileContext& context,
                                        const ConstRef<Sampler>& sampler) const
  {
    MT_ASSERT(sampler != nullptr);
    setSampler(context, *sampler);
  }

  inline bool ResourceBinding::empty() const noexcept
  {
    return _empty;
  }

  inline const TechniqueResource* ResourceBinding::resource() const noexcept
  {
    return Observer::resource();
  }

  inline void ResourceBinding::setResource(
                                      const Ref<TechniqueResource>& theResource)
  {
    setResource(theResource.get());
  }

  inline void ResourceBinding::setResource(
                                const ConstRef<TechniqueResource>& theResource)
  {
    setResource(theResource.get());
  }

  inline ResourceBindingImpl::ResourceBindingImpl(
                                  const char* name,
                                  const Technique& technique,
                                  size_t& revisionCounter,
                                  const TechniqueConfiguration* configuration) :
    ResourceBinding(name, technique, revisionCounter, configuration)
  {
  }

  inline void ResourceBindingImpl::setConfiguration(
                                    const TechniqueConfiguration* configuration)
  {
    _bindToConfiguration(configuration);
  }

  inline const TechniqueConfiguration::Resource*
                            ResourceBindingImpl::description() const noexcept
  {
    return _description;
  }

  inline bool ResourceBindingImpl::isDefault() const noexcept
  {
    return _defaultValue;
  }

  inline void ResourceBindingImpl::setSamplerAsDefault(const Sampler* sampler)
  {
    _defaultValue = true;
    if (sampler == _sampler) return;
    clear();
    _sampler = ConstRef(sampler);
    if(_sampler != nullptr) _empty = false;
  }
}