#pragma once

#include <optional>

#include <vulkan/vulkan.h>

#include <vkr/ImageSlice.h>

namespace mt
{
  //  Структура используется во время заполнения буфера команд для
  //  для отслеживания, в каком лэйауте будет находится Image и/или его
  //  отдельные части. А так же при сшивании буферов команд для определения,
  //  какие требования по лэйаутам и барьерам предъявляются к предыдущим и
  //  последующим буферам команд.
  struct ImageLayoutState
  {
    //  В каком лэйауте должен находиться Image перед стартом буфера команд
    VkImageLayout requiredIncomingLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  В каком лэйауте будет находиться Image после отработки всех команд
    VkImageLayout outcomingLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  Если isLayoutChanged == true, значит внутрь буфера команд был добавлен
    //  хотя бы один image барьер на этом Image
    bool isLayoutChanged = false;

    //  Маска стадий пайплайна, на которых читается Image в буфере команд
    //  до первого преобразования лэйаута
    VkPipelineStageFlags readStagesMask = VK_PIPELINE_STAGE_NONE;

    //  Маска типов доступа на чтение, то есть те кэши, которые надо
    //  ивалидировать перед использованием буфера команд
    VkAccessFlags readAccessMask = VK_ACCESS_NONE;

    //  Маска стадий пайплайна, на которых записывается Image в буфере команд
    //  после последнего преобразования лэйаута
    VkPipelineStageFlags writeStagesMask = VK_PIPELINE_STAGE_NONE;

    //  Маска типов доступа на запись, то есть те кэши, которые надо флашить
    //  после использованием буфера команд
    VkAccessFlags writeAccessMask = VK_ACCESS_NONE;

    //  Если внутри Image содержится слайс с Layout-ом, отличным от 
    //    outcomingLayout, то здесь будет храниться описание этого слайса.
    //  CommandProducer обязан убрать слайс во время финализации, так что
    //    changedSlice при передаче буфера команд в очередь должен быть
    //    nullopt
    std::optional<ImageSlice> changedSlice;
    //  Лэйаут changedSlice
    VkImageLayout sliceLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  Проверить, необходимо ли менять лэйаут Image для того, чтобы получить
    //  слайс в нужном лэйауте, или Image уже находится в подходящем
    //  состоянии.
    inline bool needToChangeLayout(
                              const ImageSlice& slice,
                              VkImageLayout requiredSliceLayout) const noexcept;
  };

  inline bool ImageLayoutState::needToChangeLayout(
                              const ImageSlice& slice,
                              VkImageLayout requiredSliceLayout) const noexcept
  {
    if(!changedSlice.has_value())
    {
      // Весь Image целиком в одном лэйауте
      return outcomingLayout != requiredSliceLayout;
    }

    if (outcomingLayout == requiredSliceLayout)
    {
      // Вся картинка в нужном нам лэйауте, но есть слайс в другом лэйауте
      return slice.isIntersected(*changedSlice);
    }

    if(sliceLayout == requiredSliceLayout)
    {
      // Вся картика в ненужном лэйауте, но есть слайс в нужном
      return !changedSlice->contains(slice);
    }

    return true;
  }
}