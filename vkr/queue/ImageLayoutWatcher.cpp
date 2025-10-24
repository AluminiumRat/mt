#include <vkr/queue/CommandBuffer.h>
#include <vkr/queue/ImageLayoutWatcher.h>
#include <vkr/Image.h>
#include <vkr/ImageSlice.h>

using namespace mt;

ImageLayoutWatcher::ImageLayoutWatcher() noexcept :
  _imageStates(1087)
{
}

void ImageLayoutWatcher::addImageUsage( const ImageSlice& slice,
                                        VkImageLayout requiredLayout,
                                        VkPipelineStageFlags readStagesMask,
                                        VkAccessFlags readAccessMask,
                                        VkPipelineStageFlags writeStagesMask,
                                        VkAccessFlags writeAccessMask,
                                        const CommandBuffer& commandBuffer)
{
  if(slice.image().isLayoutAutoControlEnabled())
  {
    auto insertion = _imageStates.emplace(&slice.image(),
                                          ImageLayoutStateInBuffer());
    bool isNewRecord = insertion.second;
    ImageLayoutStateInBuffer& imageState = insertion.first->second;
    if(isNewRecord)
    {
      // Image ещё не использовался в этом комманд буфере, просто заполняем
      // требования по входному лэйоуту. Преобразование будет делать очередь
      // команд при сшивании буферов команд.
      imageState.requiredIncomingLayout = requiredLayout;
      imageState.outcomingLayout = requiredLayout;
      imageState.readStagesMask = readStagesMask;
      imageState.readAccessMask = readAccessMask;
      imageState.writeStagesMask = writeStagesMask;
      imageState.writeAccessMask = writeAccessMask;
    }
    else
    {
      // Image уже использовался в этом буфере команд
      if(imageState.needToChangeLayout(slice, requiredLayout))
      {
        _addImageLayoutTransform( slice,
                                  imageState,
                                  requiredLayout,
                                  readStagesMask,
                                  readAccessMask,
                                  writeStagesMask,
                                  writeAccessMask,
                                  commandBuffer);
      }
      else
      {
        // Не надо преобразовавать лэйауты,просто ужесточаем требования по
        // барьерам до и после буфера команд.
        imageState.writeStagesMask |= writeStagesMask;
        imageState.writeAccessMask |= writeAccessMask;
        if(!imageState.isLayoutChanged)
        {
          imageState.readStagesMask |= readStagesMask;
          imageState.readAccessMask |= readAccessMask;
        }
      }
    }
  }
}

void ImageLayoutWatcher::_addImageLayoutTransform(
                                          const ImageSlice& slice,
                                          ImageLayoutStateInBuffer& imageState,
                                          VkImageLayout requiredLayout,
                                          VkPipelineStageFlags readStagesMask,
                                          VkAccessFlags readAccessMask,
                                          VkPipelineStageFlags writeStagesMask,
                                          VkAccessFlags writeAccessMask,
                                          const CommandBuffer& commandBuffer)
{
  if(imageState.changedSlice.has_value())
  {
    // Если в image есть поменянный слайс - преобразование с откатом слайса
    commandBuffer.imageBarrier( *imageState.changedSlice,
                                imageState.sliceLayout,
                                imageState.outcomingLayout,
                                imageState.writeStagesMask,
                                readStagesMask,
                                imageState.writeAccessMask,
                                VK_ACCESS_NONE);
    // Основное преобразование лэйаута
    commandBuffer.imageBarrier( slice,
                                imageState.outcomingLayout,
                                requiredLayout,
                                imageState.writeStagesMask,
                                readStagesMask,
                                VK_ACCESS_NONE,
                                readAccessMask);
    imageState.changedSlice.reset();
  }
  else
  {
    // Преобразование без отката слайса
    commandBuffer.imageBarrier( slice,
                                imageState.outcomingLayout,
                                requiredLayout,
                                imageState.writeStagesMask,
                                readStagesMask,
                                imageState.writeAccessMask,
                                readAccessMask);
  }

  // Обновляем информацию по image-у
  if(slice.isSliceFull())
  {
    // Мы только что поменяли лэйаут для всего image
    imageState.outcomingLayout = requiredLayout;
  }
  else
  {
    // Мы поменяли только часть image
    imageState.changedSlice = slice;
    imageState.sliceLayout = requiredLayout;
  }

  // Мы только что прошли через барьер памяти, поэтому маски на флаш кэша
  // после буфера полностью обновляются
  imageState.writeStagesMask = writeStagesMask;
  imageState.writeAccessMask = writeAccessMask;

  imageState.isLayoutChanged = true;
}