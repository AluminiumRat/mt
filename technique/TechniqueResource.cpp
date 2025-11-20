#include <technique/TechniqueResource.h>
#include <technique/Technique.h>
#include <util/Assert.h>
#include <util/Log.h>
#include <vkr/DescriptorSet.h>

using namespace mt;

TechniqueResource::TechniqueResource(
                                  const char* name,
                                  const Technique& technique,
                                  size_t& revisionCounter,
                                  const TechniqueConfiguration* configuration) :
  _name(name),
  _technique(technique),
  _revisionCounter(revisionCounter)
{
  _bindToConfiguration(configuration);
}

void TechniqueResource::_bindToConfiguration(
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

void TechniqueResourceImpl::bindToDescriptorSet(DescriptorSet& set)
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

void TechniqueResourceImpl::_bindSampler(DescriptorSet& set)
{
  if (_sampler == nullptr)
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": sampler is null";
  }
  else set.attachSampler(*_sampler, _description->bindingIndex);
}

void TechniqueResourceImpl::_bindBuffer(DescriptorSet& set)
{
    if(_buffer == nullptr)
    {
      Log::warning() << _technique.debugName() << " : " << _name << ": buffer is null";
    }
    else
    {
      set.attachBuffer( *_buffer,
                        _description->bindingIndex,
                        _description->type,
                        0,
                        _buffer->size());
    }
}

void TechniqueResourceImpl::_bindImage(DescriptorSet& set)
{
  if (_images.empty())
  {
    Log::warning() << _technique.debugName() << " : " << _name << ": images is null";
    return;
  }

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
