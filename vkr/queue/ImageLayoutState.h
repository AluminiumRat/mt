#pragma once

#include <optional>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include <vkr/ImageSlice.h>

namespace mt
{
  //  Состояние лэйаутов Image-а, который привязан к какой-то конкретной
  //  очереди команд. Используется при сшивании буферов команд в очередь
  //  для отслеживания лэйаутов у Image-ей c автоконтролем лэйаута.
  struct ImageLayoutStateInQueue
  {
    //  В каком лэйауте будет находиться Image после выполнения всех команд
    //  из очереди.
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  Стадии пайплайна, на которых происходила запись в Image со времени
    //  последнего барьера.
    VkPipelineStageFlags writeStagesMask = VK_PIPELINE_STAGE_NONE;
    //  Типы доступа на запись, которые происходили с Image со времени
    //  последнего барьера
    VkAccessFlags writeAccessMask = VK_ACCESS_NONE;

    //  Если внутри Image содержится слайс с Layout-ом, отличным от 
    //    основного лэйаута, то здесь будет храниться описание этого слайса.
    std::optional<ImageSlice> changedSlice;
    //  Лэйаут changedSlice
    VkImageLayout sliceLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  };

  //  Состояние layout-ов одного Image в буфере команд.
  //  Структура используется во время заполнения буфера команд для
  //    для отслеживания, в каком лэйауте будет находится Image и/или его
  //    отдельные части. А так же при сшивании буферов команд перед отправкой
  //    их в очередь команд, для определения, какие требования по лэйаутам и
  //    барьерам предъявляются к предыдущим и последующим буферам команд.
  struct ImageLayoutStateInBuffer
  {
    //  В каком лэйауте должен находиться Image перед стартом буфера команд
    VkImageLayout requiredIncomingLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  В каком лэйауте будет находиться Image после отработки всех команд из
    //  буфера
    VkImageLayout outcomingLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //  Если isLayoutChanged == true, значит внутрь буфера команд был добавлен
    //    хотя бы один image барьер на этом Image
    bool isLayoutChanged = false;

    //  Cтадиb пайплайна, на которых читается Image в буфере команд
    //  до первого преобразования лэйаута.
    VkPipelineStageFlags readStagesMask = VK_PIPELINE_STAGE_NONE;

    //  Типы доступа на чтение, то есть те кэши, которые надо ивалидировать
    //  перед использованием буфера команд
    VkAccessFlags readAccessMask = VK_ACCESS_NONE;

    //  Стадии пайплайна, на которых записываются данные в Image после
    //  последнего преобразования лэйаута
    VkPipelineStageFlags writeStagesMask = VK_PIPELINE_STAGE_NONE;

    //  Типы доступа на запись после последнего преобразования лэйаута,
    //  то есть те кэши, которые надо флашить после использованием буфера команд
    VkAccessFlags writeAccessMask = VK_ACCESS_NONE;

    //  Если внутри Image содержится слайс с Layout-ом, отличным от 
    //    outcomingLayout, то здесь будет храниться описание этого слайса.
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

  //  Набор соответствий Image -> состояние лэйоутов
  using ImageLayoutStateSet =
                    std::unordered_map<const Image*, ImageLayoutStateInBuffer>;

  inline bool ImageLayoutStateInBuffer::needToChangeLayout(
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