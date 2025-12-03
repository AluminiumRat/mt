#pragma once

#include <vulkan/vulkan.h>

namespace mt
{
  //  Некоторые свойства форматов VkFormat
  struct ImageFormatFeatures
  {
    std::string name;
    VkFormat format;

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

    //  Размер одного текселя в битах. Для сжатых ворматов размер выводится
    //    как средний размер по блоку
    //  Имеет значение только при передаче данных на/из ГПУ для вычисления
    //    размера буферов
    uint32_t texelSize;

    //  true для блочных форматов (bc1, bc2, bc3, bc4, bc5, bc6 и bc7)
    bool isCompressed;
  };

  const ImageFormatFeatures& getFormatFeatures(VkFormat format);
  const ImageFormatFeatures& getFormatFeatures(const std::string& formatName);
}