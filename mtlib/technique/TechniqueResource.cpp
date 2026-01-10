#include <algorithm>
#include <mutex>
#include <stdexcept>

#include <technique/TechniqueResource.h>
#include <util/Assert.h>
#include <util/Log.h>

using namespace mt;

TechniqueResource::Observer::~Observer() noexcept
{
  detach();
}

void TechniqueResource::Observer::detach() noexcept
{
  if(_resource != nullptr)
  {
    _resource->removeObserver(*this);
    _resource = nullptr;
  }
}

void TechniqueResource::Observer::attach(const TechniqueResource* resource)
{
  detach();
  if(resource != nullptr)
  {
    resource->addObserver(*this);
    _resource = resource;
  }
}

void TechniqueResource::Observer::onResourceUpdated()
{
}

void TechniqueResource::setImage(const ImageView* image)
{
  if(_image == image) return;
  _image= image;
  _notifyObservers();
}

void TechniqueResource::setBuffer(const DataBuffer* buffer)
{
  if (_buffer == buffer) return;
  _buffer = buffer;
  _notifyObservers();
}

void TechniqueResource::setSampler(const Sampler* sampler)
{
  if (_sampler == sampler) return;
  _sampler = sampler;
  _notifyObservers();
}

void TechniqueResource::addObserver(Observer& observer) const
{
  std::lock_guard lock(_observersMutex);
  Observers::const_iterator iObserver = std::find(_observers.begin(),
                                                  _observers.end(),
                                                  &observer);
  MT_ASSERT(iObserver == _observers.end());
  _observers.push_back(&observer);
}

void TechniqueResource::removeObserver(Observer& observer) const noexcept
{
  std::lock_guard lock(_observersMutex);
  Observers::const_iterator iObserver = std::find(_observers.begin(),
                                                  _observers.end(),
                                                  &observer);
  MT_ASSERT(iObserver != _observers.end());
  _observers.erase(iObserver);
}

void TechniqueResource::_notifyObservers() const noexcept
{
  std::lock_guard lock(_observersMutex);
  for(Observer* observer : _observers)
  {
    try
    {
      observer->onResourceUpdated();
    }
    catch(std::exception& error)
    {
      Log::error() << "TechniqueResource::_notifyObservers: " << error.what();
    }
  }
}
