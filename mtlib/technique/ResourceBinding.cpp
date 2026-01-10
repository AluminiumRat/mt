#include <technique/ResourceBinding.h>
#include <technique/Technique.h>
#include <technique/TechniqueVolatileContext.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/pipeline/DescriptorSet.h>

using namespace mt;

ResourceBinding::ResourceBinding( const char* name,
                                  const Technique& technique,
                                  size_t& revisionCounter,
                                  const TechniqueConfiguration* configuration) :
  _name(name),
  _technique(technique),
  _revisionCounter(revisionCounter),
  _empty(true),
  _defaultValue(true)
{
  _bindToConfiguration(configuration);
}

void ResourceBinding::_bindToConfiguration(
                                    const TechniqueConfiguration* configuration)
{
  //  Просто ищем описание ресурсф в конфигурации и убеждаемся, что оно нам
  //  подходит
  _description = nullptr;
  if(configuration != nullptr)
  {
    for(const TechniqueConfiguration::Resource& description :
                                                      configuration->resources)
    {
      if(description.name == _name)
      {
        if( description.set != DescriptorSetType::COMMON &&
            description.type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
          _description = &description;
          if(_description->set == DescriptorSetType::STATIC)
          {
            _revisionCounter++;
          }
        }
        return;
      }
    }
  }
}

void ResourceBinding::setBuffer(TechniqueVolatileContext& context,
                                const DataBuffer& buffer) const
{
  if(_description == nullptr) return;
  if(_description->set != DescriptorSetType::VOLATILE)
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": resource is not volatile";
    return;
  }

  context.descriptorSet->attachBuffer( buffer,
                                      _description->bindingIndex,
                                      _description->type,
                                      0,
                                      _buffer->size());
  if(empty()) context.resourcesBinded++;
}

void ResourceBinding::setImage( TechniqueVolatileContext& context,
                                const ImageView& image) const
{
  if(_description == nullptr) return;
  if(_description->set != DescriptorSetType::VOLATILE)
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": resource is not volatile";
    return;
  }

  VkImageLayout layout = _description->writeAccess ?
                                    VK_IMAGE_LAYOUT_GENERAL:
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  context.descriptorSet->attachImage( image,
                                      _description->bindingIndex,
                                      _description->type,
                                      _description->pipelineStages,
                                      _description->writeAccess,
                                      layout);
  if(empty()) context.resourcesBinded++;
}

void ResourceBinding::setImages(
                            TechniqueVolatileContext& context,
                            std::span<const ConstRef<ImageView>> images) const
{
  MT_ASSERT(!images.empty());
  if(_description == nullptr) return;
  if(_description->set != DescriptorSetType::VOLATILE)
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": resource is not volatile";
    return;
  }

  VkImageLayout layout = _description->writeAccess ?
                                    VK_IMAGE_LAYOUT_GENERAL:
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if(images.size() == 1)
  {
    context.descriptorSet->attachImage( *images[0],
                                        _description->bindingIndex,
                                        _description->type,
                                        _description->pipelineStages,
                                        _description->writeAccess,
                                        layout);
  }
  else
  {
    uint32_t imagesCount = std::min(uint32_t(images.size()),
                                    _description->count);
    std::span imagesSpan(images.data(), imagesCount);
    context.descriptorSet->attachImages(imagesSpan,
                                        _description->bindingIndex,
                                        _description->type,
                                        _description->pipelineStages,
                                        _description->writeAccess,
                                        layout);
  }
  if(empty()) context.resourcesBinded++;
}

void ResourceBinding::setSampler( TechniqueVolatileContext& context,
                                  const Sampler& sampler) const
{
  if(_description == nullptr) return;
  if(_description->set != DescriptorSetType::VOLATILE)
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": resource is not volatile";
    return;
  }
  context.descriptorSet->attachSampler(sampler, _description->bindingIndex);
  if(empty()) context.resourcesBinded++;
}

void ResourceBinding::setResource(const TechniqueResource* theResource)
{
  _defaultValue = false;
  if(resource() == theResource) return;
  clear();
  attach(theResource);
  if(theResource != nullptr) onResourceUpdated();
}

void ResourceBinding::onResourceUpdated()
{
  MT_ASSERT(resource() != nullptr);

  _buffer.reset();
  _images.clear();
  _sampler.reset();
  _empty = true;

  if(resource()->buffer() != nullptr)
  {
    _buffer = ConstRef(resource()->buffer());
    _empty = false;
  }
  else if(resource()->image() != nullptr)
  {
    _images.push_back(ConstRef(resource()->image()));
    _empty = false;
  }
  else if(resource()->sampler() != nullptr)
  {
    _sampler = ConstRef(resource()->sampler());
    _empty = false;
  }

  if( _description != nullptr &&
      _description->set == DescriptorSetType::STATIC)
  {
    _revisionCounter++;
  }
}

void ResourceBindingImpl::bindToDescriptorSet(DescriptorSet& set) const
{
  MT_ASSERT(_description != nullptr);

  switch(_description->type)
  {
  case VK_DESCRIPTOR_TYPE_SAMPLER:
    _bindSampler(set);
    break;

  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    _bindBuffer(set);
    break;

  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    _bindImage(set);
    break;

  default:
    Log::warning() << _technique.debugName() << " : " << _name << ": unsupported resource type";
  }
}

void ResourceBindingImpl::_bindSampler(DescriptorSet& set) const
{
  if (_sampler == nullptr) return;
  set.attachSampler(*_sampler, _description->bindingIndex);
}

void ResourceBindingImpl::_bindBuffer(DescriptorSet& set) const
{
    if(_buffer == nullptr) return;
    set.attachBuffer( *_buffer,
                      _description->bindingIndex,
                      _description->type,
                      0,
                      _buffer->size());
}

void ResourceBindingImpl::_bindImage(DescriptorSet& set) const
{
  if (_images.empty()) return;

  VkImageLayout layout = _description->writeAccess ?
                                    VK_IMAGE_LAYOUT_GENERAL:
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if(_images.size() == 1)
  {
    set.attachImage(*_images[0],
                    _description->bindingIndex,
                    _description->type,
                    _description->pipelineStages,
                    _description->writeAccess,
                    layout);
  }
  else
  {
    uint32_t imagesCount = std::min(uint32_t(_images.size()),
                                    _description->count);
    std::span imagesSpan(_images.data(), imagesCount);
    set.attachImages(imagesSpan,
                    _description->bindingIndex,
                    _description->type,
                    _description->pipelineStages,
                    _description->writeAccess,
                    layout);
  }
}
