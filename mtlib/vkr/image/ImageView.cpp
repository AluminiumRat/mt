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
  _createHandle();
}

ImageView::ImageView(const Image& image) :
  _handle(VK_NULL_HANDLE),
  _image(&image),
  _slice(image),
  _viewType(_getDefaultViewType(image)),
  _components(defaultComponentMapping)
{
  _createHandle();
}

void ImageView::_createHandle()
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = _image->handle();
  createInfo.viewType = _viewType;
  createInfo.format = _image->format();
  createInfo.components = _components;
  createInfo.subresourceRange = _slice.makeRange();

  if (vkCreateImageView(_image->device().handle(),
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

VkImageViewType ImageView::_getDefaultViewType(const Image& image) noexcept
{
  if(image.arraySize() == 1)
  {
    switch(image.imageType())
    {
    case VK_IMAGE_TYPE_1D:
      return VK_IMAGE_VIEW_TYPE_1D;
    case VK_IMAGE_TYPE_2D:
      return VK_IMAGE_VIEW_TYPE_2D;
    case VK_IMAGE_TYPE_3D:
      return VK_IMAGE_VIEW_TYPE_3D;
    default:
      return VK_IMAGE_VIEW_TYPE_2D;
    }
  }
  switch(image.imageType())
  {
  case VK_IMAGE_TYPE_1D:
    return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
  case VK_IMAGE_TYPE_2D:
    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  case VK_IMAGE_TYPE_3D:
    return VK_IMAGE_VIEW_TYPE_3D;
  default:
    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  }
}
