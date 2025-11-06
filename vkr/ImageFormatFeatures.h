#pragma once

#include <vulkan/vulkan.h>

namespace mt
{
  //  Некоторые свойства форматов VkFormat
  struct ImageFormatFeatures
  {
    // Полный набор доступных слоев
    VkImageAspectFlags aspectMask;

    // Содержин информацию о цвете
    bool isColor;
    bool hasR;
    bool hasG;
    bool hasB;
    bool hasA;

    // Может использоваться как deptStencil рендер таргет
    bool isDepthStencil;
    bool hasDepth;
    bool hasStencil;
  };

  const ImageFormatFeatures& getFormatFeatures(VkFormat format);
}