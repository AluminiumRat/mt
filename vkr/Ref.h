#pragma once

#include <vkr/RefCntr.h>

namespace mt
{
  // Базовый класс для создания умных указателей с подсчетом количества
  // ссылок внутри объекта. Работает в паре с RefCntr(собственно, сам счетчик
  // ссылок).
  class RefCntrReference
  {
  public:
    inline RefCntrReference() noexcept;
    inline explicit RefCntrReference(const RefCntr* resource) noexcept;
    inline RefCntrReference(const RefCntrReference& other) noexcept;
    inline RefCntrReference(RefCntrReference&& other) noexcept;
    inline RefCntrReference& operator = (const RefCntr* resource) noexcept;
    inline RefCntrReference& operator = (
                                      const RefCntrReference& other) noexcept;
    inline RefCntrReference& operator = (RefCntrReference&& other) noexcept;
    inline ~RefCntrReference() noexcept;

    inline void reset() noexcept;

    explicit operator bool() const noexcept;

  protected:
    inline const RefCntr* resource() const noexcept;
    inline void unref() noexcept;

  private:
    const RefCntr* _resource;
  };

  // Концепт потомка от RefCntr
  template <typename Resource>
  concept RefCntrChild = std::is_base_of<RefCntr, Resource>::value;

  // Умный указатель на неконстантный объект
  template <RefCntrChild Resource>
  class Ref : public RefCntrReference
  {
  public:
    inline Ref() noexcept;
    inline explicit Ref(Resource* resource) noexcept;
    template <RefCntrChild OtherResourceType>
    inline Ref(const Ref<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline Ref(Ref<OtherResourceType>&& other) noexcept;
    inline Ref& operator = (Resource* resource) noexcept;
    template <RefCntrChild OtherResourceType>
    inline Ref& operator = (const Ref<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline Ref& operator = (Ref<OtherResourceType>&& other) noexcept;
    ~Ref() = default;

    inline Resource* get() const noexcept;
    inline Resource& operator*() const noexcept;
    inline Resource* operator->() const noexcept;
  };
  template <RefCntrChild Resource>
  inline bool operator == (const Ref<Resource>& x, nullptr_t y);
  template <RefCntrChild Resource>
  inline bool operator != (const Ref<Resource>& x, nullptr_t y);

  // Умный указатель на константный объект
  template <RefCntrChild Resource>
  class ConstRef : public RefCntrReference
  {
  public:
    inline ConstRef() noexcept;
    inline explicit ConstRef(const Resource* resource) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef(const ConstRef<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef(const Ref<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef(ConstRef<OtherResourceType>&& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef(Ref<OtherResourceType>&& other) noexcept;
    inline ConstRef& operator = (const Resource* resource) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef& operator = (
                            const ConstRef<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef& operator = (const Ref<OtherResourceType>& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef& operator = (ConstRef<OtherResourceType>&& other) noexcept;
    template <RefCntrChild OtherResourceType>
    inline ConstRef& operator = (Ref<OtherResourceType>&& other) noexcept;
    ~ConstRef() = default;

    inline const Resource* get() const noexcept;
    inline const Resource& operator*() const noexcept;
    inline const Resource* operator->() const noexcept;
  };
  template <RefCntrChild Resource>
  inline bool operator == (const ConstRef<Resource>& x, nullptr_t y);
  template <RefCntrChild Resource>
  inline bool operator != (const ConstRef<Resource>& x, nullptr_t y);

  //--------------------------------------------------------------------------
  // Дальше идут реализации методов
  inline RefCntrReference::RefCntrReference() noexcept:
    _resource(nullptr)
  {
  }

  inline RefCntrReference::RefCntrReference(const RefCntr* resource) noexcept:
    _resource(resource)
  {
    if(_resource != nullptr) _resource->_incrementCounter();
  }

  inline RefCntrReference::RefCntrReference(
                                      const RefCntrReference& other) noexcept:
    _resource(other._resource)
  {
    if (_resource != nullptr) _resource->_incrementCounter();
  }

  inline RefCntrReference::RefCntrReference(
                                            RefCntrReference&& other) noexcept:
    _resource(other._resource)
  {
    other._resource = nullptr;
  }

  inline RefCntrReference& RefCntrReference::operator = (
                                            const RefCntr* resource) noexcept
  {
    unref();

    _resource = resource;
    if (_resource != nullptr) _resource->_incrementCounter();

    return *this;
  }

  inline RefCntrReference& RefCntrReference::operator = (
                                        const RefCntrReference& other) noexcept
  {
    unref();

    _resource = other._resource;
    if (_resource != nullptr) _resource->_incrementCounter();

    return *this;
  }

  inline RefCntrReference& RefCntrReference::operator = (
                                            RefCntrReference&& other) noexcept
  {
    unref();

    _resource = other._resource;
    other._resource = nullptr;

    return *this;
  }

  inline RefCntrReference::~RefCntrReference() noexcept
  {
    unref();
  }

  inline void RefCntrReference::reset() noexcept
  {
    unref();
  }

  RefCntrReference::operator bool() const noexcept
  {
    return resource() != nullptr;
  }

  inline void RefCntrReference::unref() noexcept
  {
    if(_resource == nullptr) return;
    int counter = _resource->_decrementCounter();
    if(counter == 0) delete _resource;
    _resource = nullptr;
  }

  inline const RefCntr* RefCntrReference::resource() const noexcept
  {
    return _resource;
  }

  template <RefCntrChild Resource>
  inline Ref<Resource>::Ref() noexcept
  {
  }

  template <RefCntrChild Resource>
  inline Ref<Resource>::Ref(Resource* resource) noexcept :
    RefCntrReference(resource)
  {
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline Ref<Resource>::Ref(const Ref<OtherResourceType>& other) noexcept :
    RefCntrReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline Ref<Resource>::Ref(Ref<OtherResourceType>&& other) noexcept :
    RefCntrReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  inline Ref<Resource>& Ref<Resource>::operator = (Resource* resource) noexcept
  {
    RefCntrReference::operator=(resource);
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline Ref<Resource>& Ref<Resource>::operator = (
                                  const Ref<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(other);
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline Ref<Resource>& Ref<Resource>::operator = (
                                       Ref<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(std::move(other));
    return *this;
  }

  template <RefCntrChild Resource>
  inline Resource* Ref<Resource>::get() const noexcept
  {
    return static_cast<Resource*>(const_cast<RefCntr*>(resource()));
  }

  template <RefCntrChild Resource>
  inline Resource* Ref<Resource>::operator->() const noexcept
  {
    return static_cast<Resource*>(const_cast<RefCntr*>(resource()));
  }

  template <RefCntrChild Resource>
  inline Resource& Ref<Resource>::operator*() const noexcept
  {
    return *static_cast<Resource*>(const_cast<RefCntr*>(resource()));
  }

  template <RefCntrChild Resource>
  inline bool operator == (const Ref<Resource>& x, nullptr_t y)
  {
    return x.get() == y;
  }

  template <RefCntrChild Resource>
  inline bool operator != (const Ref<Resource>& x, nullptr_t y)
  {
    return x.get() != y;
  }

  template <RefCntrChild Resource>
  inline ConstRef<Resource>::ConstRef() noexcept
  {
  }

  template <RefCntrChild Resource>
  inline ConstRef<Resource>::ConstRef(const Resource* resource) noexcept :
    RefCntrReference(resource)
  {
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                            const ConstRef<OtherResourceType>& other) noexcept :
    RefCntrReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                                const Ref<OtherResourceType>& other) noexcept :
    RefCntrReference(other)
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>::ConstRef(
                                ConstRef<OtherResourceType>&& other) noexcept :
    RefCntrReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>::ConstRef(Ref<OtherResourceType>&& other) noexcept :
    RefCntrReference(std::move(other))
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
  }

  template <RefCntrChild Resource>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                              const Resource* resource) noexcept
  {
    RefCntrReference::operator=(resource);
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                            const ConstRef<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(other);
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                  const Ref<OtherResourceType>& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(other);
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                   ConstRef<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(std::move(other));
    return *this;
  }

  template <RefCntrChild Resource>
  template <RefCntrChild OtherResourceType>
  inline ConstRef<Resource>& ConstRef<Resource>::operator = (
                                        Ref<OtherResourceType>&& other) noexcept
  {
    static_assert(std::is_base_of<Resource, OtherResourceType>::value, "Type cast error");
    RefCntrReference::operator=(std::move(other));
    return *this;
  }

  template <RefCntrChild Resource>
  inline const Resource* ConstRef<Resource>::get() const noexcept
  {
    return static_cast<const Resource*>(resource());
  }

  template <RefCntrChild Resource>
  inline const Resource* ConstRef<Resource>::operator->() const noexcept
  {
    return static_cast<const Resource*>(resource());
  }

  template <RefCntrChild Resource>
  inline const Resource& ConstRef<Resource>::operator*() const noexcept
  {
    return *static_cast<const Resource*>(resource());
  }

  template <RefCntrChild Resource>
  inline bool operator == (const ConstRef<Resource>& x, nullptr_t y)
  {
    return x.get() == y;
  }

  template <RefCntrChild Resource>
  inline bool operator != (const ConstRef<Resource>& x, nullptr_t y)
  {
    return x.get() != y;
  }
}