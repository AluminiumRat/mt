#pragma once

#include <util/Ref.h>
#include <vkr/image/Image.h>
#include <vkr/image/ImageView.h>
#include <vkr/pipeline/DescriptorSet.h>
#include <vkr/Sampler.h>

namespace mt
{
  class Image;
}

class TextureViewer
{
public:
  TextureViewer();
  TextureViewer(const TextureViewer&) = delete;
  TextureViewer& operator = (const TextureViewer&) = delete;
  ~TextureViewer() noexcept;

  //  image должна либо иметь включенный автоконтроль лэйаута, либо находиться
  //    в лэйауте VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  void makeGUI(const char* id, const mt::Image& image);

private:
  void _clearResources() noexcept;

private:
  mt::ConstRef<mt::Image> _lastImage;
  mt::Ref<mt::ImageView> _lastImageView;
  mt::Ref<mt::Sampler> _sampler;
  mt::Ref<mt::DescriptorSet> _descriptorSet;
};