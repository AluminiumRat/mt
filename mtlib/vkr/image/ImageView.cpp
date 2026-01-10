#include <util/Abort.h>
#include <vkr/image/ImageView.h>
#include <vkr/Device.h>

using namespace mt;

ImageView::ImageView( const Image& image,
                      const ImageSlice& slice,
                      VkImageViewType viewType,
                      const VkComponentMapping& components) :
  _handle(VK_NULL_HANDLE),
  _image(&image),
  _slice(slice),
  _viewType(viewType),
  _components(components)
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image.handle();
  createInfo.viewType = viewType;
  createInfo.format = image.format();
  createInfo.components = components;
  createInfo.subresourceRange = _slice.makeRange();

  if (vkCreateImageView(image.device().handle(),
                        &createInfo,
                        nullptr,
                        &_handle) != VK_SUCCESS)
  {
    Abort("ImageView: Failed to create image view");
  }
}

ImageView::~ImageView()
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyImageView( _image->device().handle(),
                        _handle,
                        nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
