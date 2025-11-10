#pragma once

#include <vulkan/vulkan.h>

#include <util/RefCounter.h>
#include <util/Ref.h>
#include <vkr/Image.h>
#include <vkr/ImageSlice.h>

namespace mt
{
  // Обертка вокруг VkImageView
  class ImageView : public RefCounter
  {
  public:
    static constexpr VkComponentMapping defaultComponentMapping =
                                                { .r = VK_COMPONENT_SWIZZLE_R,
                                                  .g = VK_COMPONENT_SWIZZLE_G,
                                                  .b = VK_COMPONENT_SWIZZLE_B,
                                                  .a = VK_COMPONENT_SWIZZLE_A};
  public:
    ImageView(Image& image,
              const ImageSlice& slice,
              VkImageViewType viewType,
              const VkComponentMapping& components = defaultComponentMapping);
    ImageView(const ImageView&) = delete;
    ImageView& operator = (const ImageView&) = delete;
  protected:
    virtual ~ImageView();

  public:
    inline VkImageView handle() const noexcept;
    inline Image& image() const noexcept;
    const ImageSlice& slice() const noexcept;
    inline VkImageViewType viewType() const noexcept;
    inline const VkComponentMapping& components() const noexcept;

    // Разрешение произвольного mip уровня. Отчет начинается с первого уровня,
    // попавшего в ImageView
    inline glm::uvec3 extent(uint32_t mipLevel = 0) const noexcept;

  private:
    VkImageView _handle;
    Ref<Image> _image;
    ImageSlice _slice;
    VkImageViewType _viewType;
    VkComponentMapping _components;
  };

  inline VkImageView ImageView::handle() const noexcept
  {
    return _handle;
  }

  inline Image& ImageView::image() const noexcept
  {
    return *_image;
  }

  inline const ImageSlice& ImageView::slice() const noexcept
  {
    return _slice;
  }

  inline VkImageViewType ImageView::viewType() const noexcept
  {
    return _viewType;
  }

  inline const VkComponentMapping& ImageView::components() const noexcept
  {
    return _components;
  }

  inline glm::uvec3 ImageView::extent(uint32_t mipLevel) const noexcept
  {
    return _image->extent(_slice.baseMipLevel() + mipLevel);
  }
}