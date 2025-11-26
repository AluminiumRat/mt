#include <stdexcept>
#include <string>
#include <unordered_map>

#include <util/Abort.h>
#include <vkr/image/ImageFormatFeatures.h>

using namespace mt;

using FormatsMap = std::unordered_map<VkFormat, ImageFormatFeatures>;
FormatsMap formatsMap =
  {
    {VK_FORMAT_R8G8B8_UNORM,    { .name = "VK_FORMAT_R8G8B8_UNORM",
                                  .format = VK_FORMAT_R8G8B8_UNORM,
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = false,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false,
                                  .texelSize = 3}},
    {VK_FORMAT_R8G8B8_SRGB,     { .name = "VK_FORMAT_R8G8B8_SRGB",
                                  .format = VK_FORMAT_R8G8B8_SRGB,
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = false,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false,
                                  .texelSize = 3}},
    {VK_FORMAT_R8G8B8A8_UNORM,  { .name = "VK_FORMAT_R8G8B8A8_UNORM",
                                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = true,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false,
                                  .texelSize = 4}},
    {VK_FORMAT_B8G8R8A8_SRGB,   { .name = "VK_FORMAT_B8G8R8A8_SRGB",
                                  .format = VK_FORMAT_B8G8R8A8_SRGB,
                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .isColor = true,
                                  .hasR = true,
                                  .hasG = true,
                                  .hasB = true,
                                  .hasA = true,
                                  .isDepthStencil = false,
                                  .hasDepth = false,
                                  .hasStencil = false,
                                  .texelSize = 3}},

    {VK_FORMAT_D16_UNORM,       { .name = "VK_FORMAT_D16_UNORM",
                                  .format = VK_FORMAT_D16_UNORM,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false,
                                  .texelSize = 2}},
    {VK_FORMAT_X8_D24_UNORM_PACK32,
                                { .name = "VK_FORMAT_X8_D24_UNORM_PACK32",
                                  .format = VK_FORMAT_X8_D24_UNORM_PACK32,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false,
                                  .texelSize = 4}},
    {VK_FORMAT_D32_SFLOAT,      { .name = "VK_FORMAT_D32_SFLOAT",
                                  .format = VK_FORMAT_D32_SFLOAT,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = false,
                                  .texelSize = 4}},
    {VK_FORMAT_S8_UINT,         { .name = "VK_FORMAT_S8_UINT",
                                  .format = VK_FORMAT_S8_UINT,
                                  .aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = false,
                                  .hasStencil = true,
                                  .texelSize = 1}},
    {VK_FORMAT_D16_UNORM_S8_UINT,
                                { .name = "VK_FORMAT_D16_UNORM_S8_UINT",
                                  .format = VK_FORMAT_D16_UNORM_S8_UINT,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true,
                                  .texelSize = 3}},
    {VK_FORMAT_D24_UNORM_S8_UINT,
                                { .name = "VK_FORMAT_D24_UNORM_S8_UINT",
                                  .format = VK_FORMAT_D24_UNORM_S8_UINT,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true,
                                  .texelSize = 4}},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,
                                { .name = "VK_FORMAT_D32_SFLOAT_S8_UINT",
                                  .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
                                  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                                VK_IMAGE_ASPECT_STENCIL_BIT,
                                  .isColor = false,
                                  .hasR = false,
                                  .hasG = false,
                                  .hasB = false,
                                  .hasA = false,
                                  .isDepthStencil = true,
                                  .hasDepth = true,
                                  .hasStencil = true,
                                  .texelSize = 8}}
  };

const ImageFormatFeatures& mt::getFormatFeatures(VkFormat format)
{
  FormatsMap::const_iterator iFeature = formatsMap.find(format);
  if(iFeature == formatsMap.end()) throw std::runtime_error("ImageFormatFeatures: unable to find format " + std::to_string(format));
  return iFeature->second;
}

const ImageFormatFeatures& mt::getFormatFeatures(const std::string& formatName)
{
  for(FormatsMap::const_iterator iFeature = formatsMap.begin();
      iFeature != formatsMap.end();
      iFeature++)
  {
    if(iFeature->second.name == formatName) return iFeature->second;
  }

  throw std::runtime_error("ImageFormatFeatures: unable to find format " + formatName);
}
