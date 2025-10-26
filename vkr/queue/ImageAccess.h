#pragma once
//  Здесь лежит набор мелких вспомогательных структур и функций, описывающих
//    доступ к памяти Image в контексту layout-ов и кэшей.
//  Используется для автоматической расстановки барьеров при работе с Image-ми,
//    для которых включен автоконтроль лэйаутов

#include <optional>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include <vkr/ImageSlice.h>

namespace mt
{
  class CommandBuffer;
  class Image;

  // Описание доступа к памяти ресурса во время каких-либо операций
  struct ResourceAccess
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
    inline bool needBarrier(const ResourceAccess& nextAccess) const noexcept;

    //  Создает в commandBuffer барьер памяти, разделяющий два доступа
    void createBarrier( const ResourceAccess& nextAccess,
                        const CommandBuffer& commandBuffer) const noexcept;

    //  Объединить два доступа в один (без проверок, нужен ли барьер).
    inline ResourceAccess merge(
                              const ResourceAccess& nextAccess) const noexcept;
  };

  //  Описание доступа к отдельному слайсу Image-а
  struct SliceAccess
  {
    ImageSlice slice;
    VkImageLayout requiredLayout;
    ResourceAccess memoryAccess;
  };

  // Описание того, в каких лэйаутах находится Image и какой-то отдельный
  // его слайс(не больше одного слайса на Image)
  struct ImageLayoutState
  {
    VkImageLayout primaryLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::optional<ImageSlice> changedSlice;
    VkImageLayout changedSliceLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  };

  //  Нужно ли удалить выделенный слайс для того чтобы перевести часть Image-а
  //  в нужный лэйаут
  inline bool needToCollapseSlice(const ImageLayoutState& srcState,
                                  const ImageSlice& requiredSlice,
                                  VkImageLayout requiredLayout) noexcept;

  //  Какие преобразования нужно сделать с лэйаутами Image-а, чтобы получить
  //  в нем слайс с нужным лэйаутом
  struct LayoutTranslationType
  {
    bool needToCollapseSlice = false;
    bool needToCreateSlice = false;
    inline bool needToDoAnything() const noexcept;
  };
  inline LayoutTranslationType getLayoutTranslationType(
                                const ImageLayoutState& srcState,
                                const ImageSlice& requiredSlice,
                                VkImageLayout requiredLayout) noexcept;

  // Описание изменений, которые происходят с ImageLayoutState
  struct LayoutTranslation
  {
    LayoutTranslationType translationType;
    //  Состояние лэйаутов, в которое перейдет Image, после того, как произойдет
    //  преобразование.
    ImageLayoutState nextState;
  };
  // Определить, какие изменения надо провести, чтобы в sourceState получить
  // слайс requiredSlice с лэйаутом requiredLayout
  inline LayoutTranslation getLayoutTranslation(
                                      const ImageLayoutState& sourceState,
                                      const ImageSlice& requiredSlice,
                                      VkImageLayout requiredLayout) noexcept;

  //  Описание доступа ко всему Image
  struct ImageAccess
  {
    ImageLayoutState requiredLayouts;
    ResourceAccess memoryAccess;
  };

  //  Точка, в которой нужно будет согласовать доступы к Image после того
  //  как текущий доступ будет полностью закончен
  struct ApprovingPoint
  {
    //  Буфер, в который надо будет записать барьеры
    const CommandBuffer* approvingBuffer;
    ImageAccess previousAccess;

    //  Преобразования лэйаутов, которые должны содержаться в барьерах.
    //  Известно уже на этапе создания ApprovingPoint, здесь просто временно
    //  хранится, чтобы не вычислять повторно на этапе согласования.
    LayoutTranslationType layoutTranslation;

    // Записать в approvingBuffer необходимые барьеры
    void makeApprove( const Image& image,
                      const ImageAccess& nextAccess) const noexcept;
  };

  //  Описание доступа к Image-у на протяжении работы CommandProducer-а
  struct ImageAccessLayoutState
  {
    //  Самый первый доступ к Image-у. Храним его, чтобы потом можно было сшить
    //  буферы команд внутри очереди. Пока хотябы один доступ не будет
    //  полностью сформирован, тут будет nullptr
    std::optional<ImageAccess> initialAccess;

    //  Последняя созданная точка согласования. Она уже создана, но буфер ещё не
    //  заполнен, так как текущий доступ ещё полностью не сформирован
    std::optional<ApprovingPoint> lastApprovingPoint;

    //  Текущий, ещё формируемый запрос
    ImageAccess currentAccess;
  };

  //  Соответствие Image->доступ к нему для всех отслеживаемых Image-ей
  using ImageAccessMap =
                      std::unordered_map<const Image*, ImageAccessLayoutState>;

  inline bool ResourceAccess::needBarrier(
                                const ResourceAccess& nextAccess) const noexcept
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

  inline ResourceAccess ResourceAccess::merge(
                                const ResourceAccess& nextAccess) const noexcept
  {
    return ResourceAccess{
      .readStagesMask = readStagesMask | nextAccess.readStagesMask,
      .readAccessMask = readAccessMask | nextAccess.readAccessMask,
      .writeStagesMask = writeStagesMask | nextAccess.writeStagesMask,
      .writeAccessMask = writeAccessMask | nextAccess.writeAccessMask};
  }

  inline bool needToCollapseSlice(const ImageLayoutState& srcState,
                                  const ImageSlice& requiredSlice,
                                  VkImageLayout requiredLayout) noexcept
  {
    // Если слайса нет, то и схлапывать нечего
    if(!srcState.changedSlice.has_value()) return false;

    if(srcState.changedSliceLayout == requiredLayout &&
        srcState.changedSlice->contains(requiredSlice))
    {
      // Слайс существует, но он полностью покрывает требуемый
      return false;
    }

    if(srcState.primaryLayout == requiredLayout &&
        !srcState.changedSlice->isIntersected(requiredSlice))
    {
      // Слайс существует, но требуемый слайс лежит полностью вне его
      return false;
    }

    return true;
  }

  inline bool LayoutTranslationType::needToDoAnything() const noexcept
  {
    return needToCollapseSlice || needToCreateSlice;
  }

  inline LayoutTranslationType getLayoutTranslationType(
                                        const ImageLayoutState& srcState,
                                        const ImageSlice& requiredSlice,
                                        VkImageLayout requiredLayout) noexcept
  {
    LayoutTranslationType translationType{};
    if(needToCollapseSlice(srcState, requiredSlice, requiredLayout))
    {
      // Мы будем схлапывать слайс до primaryLayout-а. Создавать новый слайс
      //  надо, если этот лэйаут нам не подходит
      translationType.needToCollapseSlice = true;
      translationType.needToCreateSlice =
                                      requiredLayout != srcState.primaryLayout;
    }
    else
    {
      //  Схлапывать существующий слайс не надо. У нас два варианта - слайс
      //    слайс существует, тогда вообще не надо ничего делать, всё и так
      //    находится в нужных лэйаутах.
      //  Если слайса нет, тогда его надо создать, если основной лэйаут нам не
      //    подходит.
      if (!srcState.changedSlice.has_value())
      {
        translationType.needToCreateSlice =
                                      requiredLayout != srcState.primaryLayout;
      }
    }
    return translationType;
  };

  inline LayoutTranslation getLayoutTranslation(
                                        const ImageLayoutState& sourceState,
                                        const ImageSlice& requiredSlice,
                                        VkImageLayout requiredLayout) noexcept
  {
    LayoutTranslation resultTranslation{};
    resultTranslation.translationType =
                                    getLayoutTranslationType( sourceState,
                                                              requiredSlice,
                                                              requiredLayout);
  
    resultTranslation.nextState = sourceState;

    if(resultTranslation.translationType.needToCollapseSlice)
    {
      resultTranslation.nextState.changedSlice.reset();
    }

    if(resultTranslation.translationType.needToCreateSlice)
    {
      if(requiredSlice.isSliceFull())
      {
        resultTranslation.nextState.primaryLayout = requiredLayout;
      }
      else
      {
        resultTranslation.nextState.changedSlice = requiredSlice;
        resultTranslation.nextState.changedSliceLayout = requiredLayout;
      }
    }

    return resultTranslation;
  }
}