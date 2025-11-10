#include <unordered_map>

#include <util/Abort.h>
#include <vkr/image/ImageFormatFeatures.h>

using namespace mt;

using FormatsMap = std::unordered_map<VkFormat, ImageFormatFeatures>;
FormatsMap formatsMap =
  {
    {VK_FORMAT_R8G8B8_UNORM,    { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = false,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false}},
    {VK_FORMAT_R8G8B8_SRGB,     { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = false,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false}},
    {VK_FORMAT_R8G8B8A8_UNORM,  { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = true,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false}},
    {VK_FORMAT_B8G8R8A8_SRGB,   { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = true,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false}},

    {VK_FORMAT_D16_UNORM,       { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false}},
    {VK_FORMAT_X8_D24_UNORM_PACK32,
                                { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false}},
    {VK_FORMAT_D32_SFLOAT,      { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false}},
    {VK_FORMAT_S8_UINT,         { .aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = false,
                                  .hasStencil = true}},
    {VK_FORMAT_D16_UNORM_S8_UINT,
                                { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true}},
    {VK_FORMAT_D24_UNORM_S8_UINT,
                                { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true}},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,
                                { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true}}
  };

const ImageFormatFeatures& mt::getFormatFeatures(VkFormat format)
{
  FormatsMap::const_iterator iFeature = formatsMap.find(format);
  if(iFeature == formatsMap.end())
  {
    Abort("Unable to find image format");
  }
  return iFeature->second;
}
