#pragma once

#include <optional>
#include <span>
#include <vector>

#include <glm/glm.hpp>

#include <vkr/ImagesAccessSet.h>
#include <vkr/ImageView.h>
#include <vkr/RefCounter.h>
#include <vkr/Ref.h>

namespace mt
{
  //  Информация о том, куда будет рисовать GraphicPipeline
  //  Основное назначение - заранее подготовить информацию, которая понадобится
  //    в цикле рендера.
  //  Сам по себе не управляет никаким вулкан хэндлом (мы используем
  //    DynamicRender), но наследуется от RefCounter, чтобы можно было захватить
  //    все связанные ресурсы за один инкремент счетчика и немного сэкономить на
  //    спичках
  class FrameBuffer : public RefCounter
  {
  public:
    //  Максимальное количество одновременно подключаемых колор таргетов
    static constexpr uint32_t maxColorAttachments = 8;

    //  Информация об отдельном колор таргете для рендера, который входит в
    //  состав фрэйм буфера
    struct ColorAttachmentInfo
    {
      //  Непосредственно сюда будут писаться новые данные.
      //  Не должен быть nullptr
      //  Соответствующий Image должен быть создан с usageFlags со включенным
      //    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
      ImageView* target;
      //  Что надо сделать со старыми данными в таргете перед началом работы
      //    с буфером
      VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      //  Что надо сделать с данными после того как работа с фрэйм буфером будет
      //    закончена.
      VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      //  Если loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, то таргет будет почищен в
      //    это значение
      VkClearColorValue clearValue = {};
      //  Если target мультисэмплинговый, то определяет, как он должен
      //    резолвится в resolveTarget.
      //  Если выставлен в VK_RESOLVE_MODE_NONE, то резолва не будет, а
      //    resolveTarget будет проигнорирован.
      VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_NONE;
      //  Куда будет резолвится мультисэмплинговый target.
      //  Если resolveMode не равен VK_RESOLVE_MODE_NONE, то resolveTarget не
      //    должен быть nullptr.
      //  Должен содердать строго 1 сэмпл на тэксель (VK_SAMPLE_COUNT_1_BIT)
      ImageView* resolveTarget = nullptr;
    };

    //  Информация об деф-стенсил таргете для рендера, который входит в
    //  состав фрэйм буфера
    struct DepthStencilAttachmentInfo
    {
      //  Непосредственно сюда будут писаться новые данные.
      //  Не должен быть nullptr
      //  Соответствующий Image должен быть создан с usageFlags со включенным
      //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
      ImageView* target;
      //  Что надо сделать со старыми данными в таргете перед началом работы
      //    с буфером
      VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      //  Что надо сделать с данными после того как работа с фрэйм буфером будет
      //    закончена.
      VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      //  Если loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, то таргет будет почищен в
      //    это значение
      VkClearDepthStencilValue clearValue = {};
    };

  public:
    FrameBuffer(std::span<const ColorAttachmentInfo> colorAttachments,
                const DepthStencilAttachmentInfo* depthStencilAttachment);
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator = (const FrameBuffer&) = delete;
  protected:
    virtual ~FrameBuffer() noexcept = default;

  public:
    inline glm::uvec2 extent() const noexcept;

    inline const std::vector<ColorAttachmentInfo>&
                                              colorAttachments() const noexcept;
    inline const DepthStencilAttachmentInfo*
                                        depthStencilAttachment() const noexcept;

    inline const VkRenderingInfoKHR& bindingInfo() const noexcept;

    //  Информация о доступе к Image со включенным автоконтролем лэйаутов
    inline const ImagesAccessSet& imagesAccess() const noexcept;

  private:
    VkRenderingAttachmentInfo _fillFrom(const ColorAttachmentInfo& info,
                                        VkImageLayout layout) const noexcept;
    VkRenderingAttachmentInfo _fillFrom(const DepthStencilAttachmentInfo& info,
                                        VkImageLayout layout) const noexcept;
    void _fillColorAttachments(
                          std::span<const ColorAttachmentInfo> colorAttachments);
    void _fillDepthStencilAttachment(
                              const DepthStencilAttachmentInfo* attachment);

  private:
    glm::uvec2 _extent;

    std::vector<ColorAttachmentInfo> _colorAttachments;
    std::optional<DepthStencilAttachmentInfo> _depthStencilAttachment;

    std::vector<VkRenderingAttachmentInfo> _vkColorAttachments;
    VkRenderingAttachmentInfo _vkDepthAttachment;
    VkRenderingAttachmentInfo _vkStencilAttachment;
    VkRenderingInfo _vkBindingInfo;

    std::vector<RefCounterReference> _lockedResources;

    ImagesAccessSet _imagesAccess;
  };

  inline glm::uvec2 FrameBuffer::extent() const noexcept
  {
    return _extent;
  }

  inline const std::vector<FrameBuffer::ColorAttachmentInfo>&
                                  FrameBuffer::colorAttachments() const noexcept
  {
    return _colorAttachments;
  }

  inline const FrameBuffer::DepthStencilAttachmentInfo*
                            FrameBuffer::depthStencilAttachment() const noexcept
  {
    if(_depthStencilAttachment.has_value())
    {
      return &_depthStencilAttachment.value();
    }
    return nullptr;
  }

  inline const VkRenderingInfoKHR& FrameBuffer::bindingInfo() const noexcept
  {
    return _vkBindingInfo;
  }

  inline const ImagesAccessSet& FrameBuffer::imagesAccess() const noexcept
  {
    return _imagesAccess;
  }
}