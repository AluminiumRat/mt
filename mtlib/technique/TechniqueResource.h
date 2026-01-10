#pragma once

#include <vector>

#include <util/RefCounter.h>
#include <util/Ref.h>
#include <util/SpinLock.h>
#include <vkr/image/ImageView.h>
#include <vkr/DataBuffer.h>
#include <vkr/Sampler.h>

namespace mt
{
  class TechniqueResource;
  class ImageView;
  class DataBuffer;
  class Sampler;

  //  Динамический источник данных для техник. Работает в паре с ResourceBinding
  //  Предоставляет технике данные, которые могут меняться со временем.
  //  Например, из-за того что были изменен файл текстуры на диске.
  //  Используется как связка между техникой и ресурс менеджерами.
  //  Потокобезопасность: сам по себе класс не является потокобезопасным, но
  //    но дает некоторые правила гарантии, позволяющие использовать его в
  //    многопоточной среде.
  //    1)  Добавление и удаление обсерверов к ресурсу потокобезопасное. Но,
  //        обратите внимание, что методы Observer::attach и Observer::detch не
  //        потокобезопасны.
  //    2)  Получать ImageView, DataBuffer и Sampler из ресурса можно в
  //        любом потоке
  //    3)  Устанавливать ImageView, DataBuffer и Sampler в ресурс можно
  //        только в синхронной части рабочего цикла приложения
  class TechniqueResource : public RefCounter
  {
  public:
    //  Абстрактный класс, который наблюдает за изменениями ресурса. От него
    //  наследуется ResourceBinding. В прочем, можно использовать по своему
    //  усмотрению.
    class Observer
    {
    public:
      Observer() = default;
      Observer(const Observer&) = delete;
      Observer& operator = (const Observer&) = delete;
      virtual ~Observer() noexcept;

    protected:
      //  Подключиться к ресурсу. Используйте для того чтобы начать наблюдение
      //    за ресурсом. Если обсервер уже следил за другим ресурсом, то
      //    произойдет отключение от старого ресурса.
      void attach(const TechniqueResource* resource);

      //  Прекратить наблюдение за текущим ресурсом
      void detach() noexcept;

      inline const TechniqueResource* resource() const noexcept;

    protected:
      friend class TechniqueResource;
      //  Обработчик сигнала о том, что ресурс поменялся. Вызывается в то
      //    же время и в том же потоке, что и само обновление ресурса.
      //  Менеджеры ресурсов обновляют ресурсы только в синхронной части
      //    основного цикла, стоит надеяться, что никто ничего здесь не поломал
      //    и дополнительной синхронизации не нужно
      virtual void onResourceUpdated();

    private:
      ConstRef<TechniqueResource> _resource;
    };

  public:
    TechniqueResource() = default;
    TechniqueResource(const TechniqueResource&) = delete;
    TechniqueResource& operator = (const TechniqueResource&) = delete;
    virtual ~TechniqueResource() noexcept = default;

    //  Установить ImageView в качестве источника данных. Обычно этим
    //    занимается менеджер текстур. Если хотите выставлять вручную,
    //    позабодьтесь о том, чтобы это происходило в синхронной части
    //    рабочего цикла приложения.
    void setImage(const ImageView* image);
    inline const ImageView* image() const noexcept;
    //  Установить буфер. Те же особенности, что и для ImageView
    void setBuffer(const DataBuffer* buffer);
    inline const DataBuffer* buffer() const noexcept;
    //  Установить сэмплер. Те же особенности, что и для ImageView
    void setSampler(const Sampler* sampler);
    inline const Sampler* sampler() const noexcept;

  private:
    //  Методы для вызова из Observer-а
    void addObserver(Observer& observer) const;
    void removeObserver(Observer& observer) const noexcept;

  private:
    void _notifyObservers() const noexcept;

  private:
    ConstRef<ImageView> _image;
    ConstRef<DataBuffer> _buffer;
    ConstRef<Sampler> _sampler;

    using Observers = std::vector<Observer*>;
    mutable Observers _observers;
    mutable SpinLock _observersMutex;
  };

  inline const TechniqueResource*
                          TechniqueResource::Observer::resource() const noexcept
  {
    return _resource.get();
  }

  inline const ImageView* TechniqueResource::image() const noexcept
  {
    return _image.get();
  }

  inline const DataBuffer* TechniqueResource::buffer() const noexcept
  {
    return _buffer.get();
  }

  inline const Sampler* TechniqueResource::sampler() const noexcept
  {
    return _sampler.get();
  }
}