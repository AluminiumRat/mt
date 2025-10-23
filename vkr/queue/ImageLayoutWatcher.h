#pragma once

#include <unordered_map>

#include <vulkan/vulkan.h>

#include <vkr/queue/ImageLayoutState.h>

namespace mt
{
  class CommandBuffer;
  class ImageSlice;

  //  Класс занимается отслеживанием лэйаутов image-ей, вставкой барьеров в
  //    буфер команд(когда надо) и формированию требований к внешним барьерам
  //    при сшивании буферов команд.
  class ImageLayoutWatcher
  {
  public:
    ImageLayoutWatcher() noexcept = default;
    ImageLayoutWatcher(const ImageLayoutWatcher&) = delete;
    ImageLayoutWatcher& operator = (const ImageLayoutWatcher&) = delete;
    ~ImageLayoutWatcher() noexcept = default;

    //  Сделать все необходимые отметки и настройки перед использованием
    //  Image в буфере команд.
    void addImageUsage( const ImageSlice& slice,
                        VkImageLayout requiredLayout,
                        VkPipelineStageFlags readStagesMask,
                        VkAccessFlags readAccessMask,
                        VkPipelineStageFlags writeStagesMask,
                        VkAccessFlags writeAccessMask,
                        const CommandBuffer& commandBuffer);

  private:
    //  Перевести слайс в нужный layout и сделать отметку об этом в imageState
    void _addImageLayoutTransform(const ImageSlice& slice,
                                  ImageLayoutState& imageState,
                                  VkImageLayout requiredLayout,
                                  VkPipelineStageFlags readStagesMask,
                                  VkAccessFlags readAccessMask,
                                  VkPipelineStageFlags writeStagesMask,
                                  VkAccessFlags writeAccessMask,
                                  const CommandBuffer& commandBuffer);
  private:
    using ImageStates = std::unordered_map<const Image*, ImageLayoutState>;
    ImageStates _imageStates;
  };
}