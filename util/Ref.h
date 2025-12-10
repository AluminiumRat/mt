#pragma once

#include <util/RefCounter.h>

namespace mt
{
  // Базовый класс для создания умных указателей с подсчетом количества
  // ссылок внутри объекта. Работает в паре с RefCounter(собственно, сам счетчик
  // ссылок).
  class RefCounterReference
  {
  public:
    inline RefCounterReference() noexcept;
    inline explicit RefCounterReference(const RefCounter* resource) noexcept;
    inline RefCounterReference(const RefCounterReference& other) noexcept;
    inline RefCounterReference(RefCounterReference&& other) noexcept;
    inline RefCounterReference& operator = (
                                          const RefCounter* resource) noexcept;
    inline RefCounterReference& operator = (
                                    const RefCounterReference& other) noexcept;
    inline RefCounterReference& operator = (
                                          RefCounterReference&& other) noexcept;
    inline ~RefCounterReference() noexcept;

    inline void reset() noexcept;

    inline explicit operator bool() const noexcept;

  protected:
    inline const RefCounter* resource() const noexcept;
    inline void unref() noexcept;

  private:
    const RefCounter* _resource;
  };

  // Концепт рандомного умного указателя. Определяем по наличию метода get
  template <typename CheckedClass>
  concept SmartRef = requires(CheckedClass object)
  {
    object.get();
  };

  // Умный указатель на неконстантный объект
  template <typename Resource>
  class Ref : public RefCounterReference
  {
  public:
    inline Ref() noexcept;
    inline explicit Ref(Resource* resource) noexcept;
    template <typename OtherResourceType>
    inline Ref(const Ref<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline Ref(Ref<OtherResourceType>&& other) noexcept;
    inline Ref& operator = (Resource* resource) noexcept;
    template <typename OtherResourceType>
    inline Ref& operator = (const Ref<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline Ref& operator = (Ref<OtherResourceType>&& other) noexcept;
    ~Ref() = default;

    inline Resource* get() const noexcept;
    inline Resource& operator*() const noexcept;
    inline Resource* operator->() const noexcept;
  };
  // Сравнение с nullptr
  template <typename Resource>
  inline bool operator == (const Ref<Resource>& x, nullptr_t y);
  template <typename Resource>
  inline bool operator != (const Ref<Resource>& x, nullptr_t y);
  // Сравнение с сырым указателем
  template <typename Resource, typename OtherResource>
  inline bool operator == (const Ref<Resource>& x, const OtherResource* y);
  template <typename Resource, typename OtherResource>
  inline bool operator != (const Ref<Resource>& x, const OtherResource* y);
  template <typename Resource, typename OtherResource>
  inline bool operator == (const Resource* x, const Ref<OtherResource>& y);
  template <typename Resource, typename OtherResource>
  inline bool operator != (const Resource* x, const Ref<OtherResource>& y);
  // Сравнение с другим умным указателем
  template <typename Resource, SmartRef OtherPointer>
  inline bool operator == (const Ref<Resource>& x, const OtherPointer& y);
  template <typename Resource, SmartRef OtherPointer>
  inline bool operator != (const Ref<Resource>& x, const OtherPointer& y);

  // Умный указатель на константный объект
  template <typename Resource>
  class ConstRef : public RefCounterReference
  {
  public:
    inline ConstRef() noexcept;
    inline explicit ConstRef(const Resource* resource) noexcept;
    template <typename OtherResourceType>
    inline ConstRef(const ConstRef<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef(const Ref<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef(ConstRef<OtherResourceType>&& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef(Ref<OtherResourceType>&& other) noexcept;
    inline ConstRef& operator = (const Resource* resource) noexcept;
    template <typename OtherResourceType>
    inline ConstRef& operator = (
                            const ConstRef<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef& operator = (const Ref<OtherResourceType>& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef& operator = (ConstRef<OtherResourceType>&& other) noexcept;
    template <typename OtherResourceType>
    inline ConstRef& operator = (Ref<OtherResourceType>&& other) noexcept;
    ~ConstRef() = default;

    inline const Resource* get() const noexcept;
    inline const Resource& operator*() const noexcept;
    inline const Resource* operator->() const noexcept;
  };
  // Сравнение с nullptr
  template <typename Resource>
  inline bool operator == (const ConstRef<Resource>& x, nullptr_t y);
  template <typename Resource>
  inline bool operator != (const ConstRef<Resource>& x, nullptr_t y);
  // Сравнение с сырым указателем
  template <typename Resource, typename OtherResource>
  inline bool operator == (const ConstRef<Resource>& x, const OtherResource* y);
  template <typename Resource, typename OtherResource>
  inline bool operator != (const ConstRef<Resource>& x, const OtherResource* y);
  template <typename Resource, typename OtherResource>
  inline bool operator == (const Resource* x, const ConstRef<OtherResource>& y);
  template <typename Resource, typename OtherResource>
  inline bool operator != (const Resource* x, const ConstRef<OtherResource>& y);
  // Сравнение с другим умным указателем
  template <typename Resource, SmartRef OtherPointer>
  inline bool operator == (const ConstRef<Resource>& x, const OtherPointer& y);
  template <typename Resource, SmartRef OtherPointer>
  inline bool operator != (const ConstRef<Resource>& x, const OtherPointer& y);

  //--------------------------------------------------------------------------
  // Дальше идут реализации методов
  inline RefCounterReference::RefCounterReference() noexcept:
    _resource(nullptr)
  {
  }

  inline RefCounterReference::RefCounterReference(
                                          const RefCounter* resource) noexcept:
    _resource(resource)
  {
    if(_resource != nullptr) _resource->_incrementCounter();
  }

  inline RefCounterReference::RefCounterReference(
                                    const RefCounterReference& other) noexcept:
    _resource(other._resource)
  {
    if (_resource != nullptr) _resource->_incrementCounter();
  }

  inline RefCounterReference::RefCounterReference(
                                          RefCounterReference&& other) noexcept:
    _resource(other._resource)
  {
    other._resource = nullptr;
  }

  inline RefCounterReference& RefCounterReference::operator = (
                                            const RefCounter* resource) noexcept
  {
    unref();

    _resource = resource;
    if (_resource != nullptr) _resource->_incrementCounter();

    return *this;
  }

  inline RefCounterReference& RefCounterReference::operator = (
                                      const RefCounterReference& other) noexcept
  {
    unref();

    _resource = other._resource;
    if (_resource != nullptr) _resource->_incrementCounter();

    return *this;
  }

  inline RefCounterReference& RefCounterReference::operator = (
                                          RefCounterReference&& other) noexcept
  {
    unref();

    _resource = other._resource;
    other._resource = nullptr;

    return *this;
  }

  inline RefCounterReference::~RefCounterReference() noexcept
  {
    unref();
  }

  inline void RefCounterReference::reset() noexcept
  {
    unref();
  }

  inline RefCounterReference::operator bool() const noexcept
  {
    return resource() != nullptr;
  }

  inline void RefCounterReference::unref() noexcept
  {
    if(_resource == nullptr) return;
    int counter = _resource->_decrementCounter();
    if(counter == 0) delete _resource;
    _resource = nullptr;
  }

  inline const RefCounter* RefCounterReference::resource() const noexcept
  {
    return _resource;
  }

  template <typename Resource>
  inline Ref<Resource>::Ref() noexcept
  {
  }

  template <typename Resource>
  inline Ref<Resource>::Ref(Resource* resource) noexcept :
    RefCounterReference(resource)
  {
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline Ref<Resource>::Ref(const Ref<OtherResourceType>& other) noexcept :
    RefCounterReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline Ref<Resource>::Ref(Ref<OtherResourceType>&& other) noexcept :
    RefCounterReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  inline Ref<Resource>& Ref<Resource>::operator = (Resource* resource) noexcept
  {
    RefCounterReference::operator=(resource);
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline Ref<Resource>& Ref<Resource>::operator = (
                                  const Ref<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(other);
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline Ref<Resource>& Ref<Resource>::operator = (
                                       Ref<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(std::move(other));
    return *this;
  }

  template <typename Resource>
  inline Resource* Ref<Resource>::get() const noexcept
  {
    return static_cast<Resource*>(const_cast<RefCounter*>(resource()));
  }

  template <typename Resource>
  inline Resource* Ref<Resource>::operator->() const noexcept
  {
    return static_cast<Resource*>(const_cast<RefCounter*>(resource()));
  }

  template <typename Resource>
  inline Resource& Ref<Resource>::operator*() const noexcept
  {
    return *static_cast<Resource*>(const_cast<RefCounter*>(resource()));
  }

  template <typename Resource, SmartRef OtherPointer>
  inline bool operator == (const Ref<Resource>& x, const OtherPointer& y)
  {
    return x.get() == y.get();
  }

  template <typename Resource, SmartRef OtherPointer>
  inline bool operator != (const Ref<Resource>& x, const OtherPointer& y)
  {
    return x.get() != y.get();
  }

  template <typename Resource, typename OtherResource>
  inline bool operator == (const Ref<Resource>& x, const OtherResource* y)
  {
    return x.get() == y;
  }

  template <typename Resource, typename OtherResource>
  inline bool operator != (const Ref<Resource>& x, const OtherResource* y)
  {
    return x.get() != y;
  }

  template <typename Resource, typename OtherResource>
  inline bool operator == (const Resource* x, const Ref<OtherResource>& y)
  {
    return x == y.get();
  }

  template <typename Resource, typename OtherResource>
  inline bool operator != (const Resource* x, const Ref<OtherResource>& y)
  {
    return x != y.get();
  }

  template <typename Resource>
  inline bool operator == (const Ref<Resource>& x, nullptr_t y)
  {
    return x.get() == y;
  }

  template <typename Resource>
  inline bool operator != (const Ref<Resource>& x, nullptr_t y)
  {
    return x.get() != y;
  }

  template <typename Resource>
  inline ConstRef<Resource>::ConstRef() noexcept
  {
  }

  template <typename Resource>
  inline ConstRef<Resource>::ConstRef(const Resource* resource) noexcept :
    RefCounterReference(resource)
  {
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                            const ConstRef<OtherResourceType>& other) noexcept :
    RefCounterReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                                const Ref<OtherResourceType>& other) noexcept :
    RefCounterReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                                ConstRef<OtherResourceType>&& other) noexcept :
    RefCounterReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>::ConstRef(Ref<OtherResourceType>&& other) noexcept :
    RefCounterReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <typename Resource>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                              const Resource* resource) noexcept
  {
    RefCounterReference::operator=(resource);
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                            const ConstRef<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(other);
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                  const Ref<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(other);
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                   ConstRef<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(std::move(other));
    return *this;
  }

  template <typename Resource>
  template <typename OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                        Ref<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCounterReference::operator=(std::move(other));
    return *this;
  }

  template <typename Resource>
  inline const Resource* ConstRef<Resource>::get() const noexcept
  {
    return static_cast<const Resource*>(resource());
  }

  template <typename Resource>
  inline const Resource* ConstRef<Resource>::operator->() const noexcept
  {
    return static_cast<const Resource*>(resource());
  }

  template <typename Resource>
  inline const Resource& ConstRef<Resource>::operator*() const noexcept
  {
    return *static_cast<const Resource*>(resource());
  }

  template <typename Resource>
  inline bool operator == (const ConstRef<Resource>& x, nullptr_t y)
  {
    return x.get() == y;
  }

  template <typename Resource>
  inline bool operator != (const ConstRef<Resource>& x, nullptr_t y)
  {
    return x.get() != y;
  }

  template <typename Resource, typename OtherResource>
  inline bool operator == (const ConstRef<Resource>& x, const OtherResource* y)
  {
    return x.get() == y;
  }

  template <typename Resource, typename OtherResource>
  inline bool operator != (const ConstRef<Resource>& x, const OtherResource* y)
  {
    return x.get() != y;
  }

  template <typename Resource, typename OtherResource>
  inline bool operator == (const Resource* x, const ConstRef<OtherResource>& y)
  {
    return x == y.get();
  }

  template <typename Resource, typename OtherResource>
  inline bool operator != (const Resource* x, const ConstRef<OtherResource>& y)
  {
    return x != y.get();
  }

  // Сравнение с другим умным указателем
  template <typename Resource, SmartRef OtherPointer>
  inline bool operator == (const ConstRef<Resource>& x, const OtherPointer& y)
  {
    return x.get() == y.get();
  }

  template <typename Resource, SmartRef OtherPointer>
  inline bool operator != (const ConstRef<Resource>& x, const OtherPointer& y)
  {
    return x.get() != y.get();
  }
}