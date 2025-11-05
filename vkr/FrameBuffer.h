#pragma once

#include <optional>
#include <span>
#include <vector>

#include <glm/glm.hpp>

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

    //  Информация об отдельном таргете для рендера, который входит в состав
    //  фрэйм буфера
    struct AttachmentInfo
    {
      //  Непосредственно сюда будут писаться новые данные.
      //  Не должен быть nullptr
      //  Соответствующий Image должен быть создан с usageFlags со включенным
      //    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT или
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
      VkClearValue clearValue = {};
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

  public:
    FrameBuffer(std::span<AttachmentInfo> colorAttachments,
                const AttachmentInfo* depthAttachment,
                const AttachmentInfo* stencilAttachment);
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator = (const FrameBuffer&) = delete;
  protected:
    virtual ~FrameBuffer() noexcept = default;

  public:
    inline glm::uvec2 extent() const noexcept;

    inline const std::vector<AttachmentInfo>& colorAttachments() const noexcept;
    inline const AttachmentInfo* depthAttachment() const noexcept;
    inline const AttachmentInfo* stencilAttachment() const noexcept;

    inline const VkRenderingInfoKHR& bindingInfo() const noexcept;

  private:
    VkRenderingAttachmentInfo _fillFrom(const AttachmentInfo& info,
                                        VkImageLayout layout) const noexcept;
    void _fillColorAttachments(std::span<AttachmentInfo> colorAttachments);
    void _fillDepthAttachment(const AttachmentInfo* depthAttachment);
    void _fillStencilAttachment(const AttachmentInfo* stencilAttachment);

  private:
    glm::uvec2 _extent;

    std::vector<AttachmentInfo> _colorAttachments;
    std::optional<AttachmentInfo> _depthAttachment;
    std::optional<AttachmentInfo> _stencilAttachment;

    std::vector<VkRenderingAttachmentInfo> _vkColorAttachments;
    VkRenderingAttachmentInfo _vkDepthAttachment;
    VkRenderingAttachmentInfo _vkStencilAttachment;
    VkRenderingInfo _vkBindingInfo;

    std::vector<RefCounterReference> _lockedResources;
  };

  inline glm::uvec2 FrameBuffer::extent() const noexcept
  {
    return _extent;
  }

  inline const std::vector<FrameBuffer::AttachmentInfo>&
                                  FrameBuffer::colorAttachments() const noexcept
  {
    return _colorAttachments;
  }

  inline const FrameBuffer::AttachmentInfo*
                                  FrameBuffer::depthAttachment() const noexcept
  {
    if(_depthAttachment.has_value()) return &_depthAttachment.value();
    return nullptr;
  }

  inline const FrameBuffer::AttachmentInfo*
                                FrameBuffer::stencilAttachment() const noexcept
  {
    if(_stencilAttachment.has_value()) return &_stencilAttachment.value();
    return nullptr;
  }

  inline const VkRenderingInfoKHR& FrameBuffer::bindingInfo() const noexcept
  {
    return _vkBindingInfo;
  }
}