#pragma once
//  Здесь лежит набор мелких вспомогательных структур и функций, описывающих
//    доступ к памяти Image в контексту layout-ов и кэшей.
//  Используется для автоматической расстановки барьеров при работе с Image-ми,
//    для которых включен автоконтроль лэйаутов

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include <vkr/ImageSlice.h>

namespace mt
{
  class CommandBuffer;
  class Image;

  // Описание доступа к памяти ресурса во время каких-либо операций
  struct MemoryAccess
  {
    // Стадии, на которых происходит чтение из ресурса
    VkPipelineStageFlags readStagesMask = VK_PIPELINE_STAGE_NONE;
    // Типы доступа на чтение
    VkAccessFlags readAccessMask = VK_ACCESS_NONE;

    // Стадии, на которых происходит запись в ресурс
    VkPipelineStageFlags writeStagesMask = VK_PIPELINE_STAGE_NONE;
    // Типы доступа на запись
    VkAccessFlags writeAccessMask = VK_ACCESS_NONE;

    //  Нужен ли барьер памяти между двумя доступами к ресурсу
    inline bool needBarrier(const MemoryAccess& nextAccess) const noexcept;

    //  Создает в commandBuffer барьер памяти, разделяющий два доступа
    void createBarrier( const MemoryAccess& nextAccess,
                        const CommandBuffer& commandBuffer) const noexcept;

    //  Добавляет к параметрам барьера памяти те маски, которые должны быть
    //  выставлены для разделения текущего доступа и nextAccess
    void updateBarrierMasks(const MemoryAccess& nextAccess,
                            VkPipelineStageFlags& srcStages,
                            VkPipelineStageFlags& dstStages,
                            VkAccessFlags& srcAccess,
                            VkAccessFlags& dstAccess) const noexcept;

    //  Объединить два доступа в один (без проверок, нужен ли барьер).
    inline MemoryAccess merge(const MemoryAccess& nextAccess) const noexcept;
  };

  //  Описание доступа ко всему Image, одновременно и описание слайсов,
  //  которые выделенны в Image
  struct ImageAccess
  {
    // Подсказака о том, как один ImageAccess можно перевести в другой
    // Используется при согласовании двух доступов в MatchingPoint
    enum TransformHint
    {
      RESET_SLICES,     // Необходимо сбросить все слайсы и пересоздать новые
      REUSE_SLICES     // К уже существующим слфйсам надо добавить новые
    };

    static constexpr uint32_t maxSlices = 4;

    std::array<ImageSlice, maxSlices> slices;
    std::array<VkImageLayout, maxSlices> layouts;
    std::array<MemoryAccess, maxSlices> memoryAccess;
    uint32_t slicesCount = 0;

    //  Проверить, что два доступа используют полностью одинаковые
    //  слайсы в одинаковых лэйаутах
    inline bool hasSameLayouts(const ImageAccess& other) const noexcept;

    // Объединить 2 доступа в 1 без использования барьеров.
    // Возвращает false, если объединить не удалось.
    bool mergeNoBarriers(const ImageAccess& nextAccess) noexcept;

    //  Сделать доступ, который полностью включает в себя nextAccess, и
    //  переносит по возможности слайсы из текущего доступа
    TransformHint mergeWithBarriers(const ImageAccess& nextAccess) noexcept;
  };

  //  Точка согласования, в которой нужно будет согласовать предыдущий и
  //  текущий доступы к Image, после того как текущий доступ будет полностью
  //  закончен
  struct MatchingPoint
  {
    //  Буфер, в который надо будет записать барьеры
    const CommandBuffer* matchingBuffer;
    ImageAccess previousAccess;
    ImageAccess::TransformHint transformHint;

    // Записать в matchingBuffer необходимые барьеры
    void makeMatch( const Image& image,
                    const ImageAccess& nextAccess) const noexcept;
  };

  //  Описание доступа к Image-у на протяжении работы CommandProducer-а
  struct ImageAccessState
  {
    //  Самый первый доступ к Image-у. Храним его, чтобы потом можно было сшить
    //  буферы команд внутри очереди. Пока хотябы один доступ не будет
    //  полностью сформирован, тут будет nullptr
    std::optional<ImageAccess> initialAccess;

    //  Последняя созданная точка согласования. Она уже создана, но буфер ещё не
    //  заполнен, так как текущий доступ ещё полностью не сформирован
    std::optional<MatchingPoint> lastMatchingPoint;

    //  Текущий, ещё формируемый запрос
    ImageAccess currentAccess;
  };

  //  Соответствие Image->доступ к нему для всех отслеживаемых Image-ей
  using ImageAccessMap = std::unordered_map<const Image*, ImageAccessState>;

  inline bool MemoryAccess::needBarrier(
                                const MemoryAccess& nextAccess) const noexcept
  {
    // Если никто ничего не пишет - то и конфликта быть не может
    if(writeAccessMask == 0 && nextAccess.writeAccessMask == 0) return false;

    if(writeAccessMask == nextAccess.writeAccessMask)
    {
      // Эти конфликты решаются output merger-ом, здесь не нужно делать
      // барьер
      if(writeAccessMask == VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) return false;
      if(writeAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
      {
        return false;
      }
    }

    return true;
  }

  inline MemoryAccess MemoryAccess::merge(
                                const MemoryAccess& nextAccess) const noexcept
  {
    return MemoryAccess{
      .readStagesMask = readStagesMask | nextAccess.readStagesMask,
      .readAccessMask = readAccessMask | nextAccess.readAccessMask,
      .writeStagesMask = writeStagesMask | nextAccess.writeStagesMask,
      .writeAccessMask = writeAccessMask | nextAccess.writeAccessMask};
  }

  inline bool ImageAccess::hasSameLayouts(
                                      const ImageAccess& other) const noexcept
  {
    if(slicesCount != other.slicesCount) return false;
    for(uint32_t i = 0; i < slicesCount; i++)
    {
      if(layouts[i] != other.layouts[i]) return false;
      if(slices[i] != other.slices[i]) return false;
    }
    return true;
  }
}