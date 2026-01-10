#include <util/Assert.h>
#include <vkr/FrameBuffer.h>

using namespace mt;

static FrameBufferFormat crateFrameBufferFormat(
          std::span<const FrameBuffer::ColorAttachmentInfo> colorAttachments,
          const FrameBuffer::DepthStencilAttachmentInfo* depthStencilAttachment)
{
  std::vector<VkFormat> colotAttachmentFormats;
  colotAttachmentFormats.reserve(colorAttachments.size());

  for (const FrameBuffer::ColorAttachmentInfo& attachment : colorAttachments)
  {
    colotAttachmentFormats.push_back(attachment.target->image().format());
  }

  VkFormat depthStencilAttachmentFormat = VK_FORMAT_UNDEFINED;
  if (depthStencilAttachment != nullptr)
  {
    depthStencilAttachmentFormat =
                              depthStencilAttachment->target->image().format();
  }

  VkSampleCountFlagBits samplesCount =
                          depthStencilAttachment != nullptr ?
                            depthStencilAttachment->target->image().samples() :
                            colorAttachments[0].target->image().samples();

  return FrameBufferFormat( colotAttachmentFormats,
                            depthStencilAttachmentFormat,
                            samplesCount);
}

FrameBuffer::FrameBuffer(
                    std::span<const ColorAttachmentInfo> colorAttachments,
                    const DepthStencilAttachmentInfo* depthStencilAttachment) :
  _format(crateFrameBufferFormat(colorAttachments, depthStencilAttachment))
{
  MT_ASSERT(!colorAttachments.empty() || depthStencilAttachment != nullptr);
  MT_ASSERT(colorAttachments.size() <= FrameBufferFormat::maxColorAttachments);

  _extent = !colorAttachments.empty() ?
                    colorAttachments[0].target->extent() :
                    depthStencilAttachment->target->extent();

  _fillColorAttachments(colorAttachments);
  _fillDepthStencilAttachment(depthStencilAttachment);

  _vkBindingInfo = VkRenderingInfo{};
  _vkBindingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  _vkBindingInfo.renderArea = VkRect2D{ .offset = {.x = 0, .y = 0},
                                        .extent = { .width = _extent.x,
                                                    .height = _extent.y}};
  _vkBindingInfo.layerCount = 1;
  _vkBindingInfo.viewMask = 0;
  _vkBindingInfo.colorAttachmentCount = uint32_t(_vkColorAttachments.size());
  if(_vkBindingInfo.colorAttachmentCount != 0)
  {
    _vkBindingInfo.pColorAttachments = _vkColorAttachments.data();
  }
  if(_vkDepthAttachment.imageView != VK_NULL_HANDLE)
  {
    _vkBindingInfo.pDepthAttachment = &_vkDepthAttachment;
  }
  if(_vkStencilAttachment.imageView != VK_NULL_HANDLE)
  {
    _vkBindingInfo.pStencilAttachment = &_vkStencilAttachment;
  }
}

void FrameBuffer::_fillColorAttachments(
                        std::span<const ColorAttachmentInfo> colorAttachments)
{
  for(const ColorAttachmentInfo& attachmentInfo : colorAttachments)
  {
    _colorAttachments.push_back(attachmentInfo);
    _vkColorAttachments.emplace_back(
          _fillFrom(attachmentInfo, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    // Оформляем доступ к основному таргету
    if(attachmentInfo.target->image().isLayoutAutoControlEnabled())
    {
      ImageAccess access{};
      access.slices[0] = attachmentInfo.target->slice();
      access.layouts[0] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      access.memoryAccess[0] = MemoryAccess{
              .readStagesMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .readAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
              .writeStagesMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .writeAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
      access.slicesCount = 1;
      if(!_imagesAccess.addImageAccess(attachmentInfo.target->image(), access))
      {
        MT_ASSERT(false && "Unable to add color target access description");
      }
    }

    // Оформляем доступ к resolveTarget-у
    if( attachmentInfo.resolveTarget != nullptr &&
        attachmentInfo.resolveTarget->image().isLayoutAutoControlEnabled())
    {
      ImageAccess access{};
      access.slices[0] = attachmentInfo.resolveTarget->slice();
      access.layouts[0] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      access.memoryAccess[0] = MemoryAccess{
              .readStagesMask = VK_PIPELINE_STAGE_NONE,
              .readAccessMask = VK_ACCESS_NONE,
              .writeStagesMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .writeAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
      access.slicesCount = 1;
      if(!_imagesAccess.addImageAccess( attachmentInfo.resolveTarget->image(),
                                        access))
      {
        MT_ASSERT(false && "Unable to add color resolve target access description");
      }
    }

    // Захватываем Image-ы, чтобы не удалились раньше времени
    _lockedResources.push_back(RefCounterReference(attachmentInfo.target));
    if (attachmentInfo.resolveTarget != nullptr)
    {
      _lockedResources.push_back(
        RefCounterReference(attachmentInfo.resolveTarget));
    }
  }
}

VkRenderingAttachmentInfo FrameBuffer::_fillFrom(
                                          const ColorAttachmentInfo& info,
                                          VkImageLayout layout) const noexcept
{
  MT_ASSERT(info.target != nullptr);
  MT_ASSERT(info.target->extent().z == 1);
  MT_ASSERT(_extent == glm::uvec2(info.target->extent()));
  MT_ASSERT(info.resolveMode == VK_RESOLVE_MODE_NONE || info.resolveTarget != nullptr);

  VkRenderingAttachmentInfo result = VkRenderingAttachmentInfo{};
  result.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  result.imageView = info.target->handle();
  result.imageLayout = layout;
  result.resolveMode = info.resolveMode;
  if(result.resolveMode != VK_RESOLVE_MODE_NONE)
  {
    result.resolveImageView = info.resolveTarget->handle();
    result.resolveImageLayout = layout;
  }
  result.loadOp = info.loadOp;
  result.storeOp = info.storeOp;
  result.clearValue = VkClearValue{.color = info.clearValue};

  return result;
}

void FrameBuffer::_fillDepthStencilAttachment(
                                  const DepthStencilAttachmentInfo* attachment)
{
  _vkDepthAttachment = VkRenderingAttachmentInfo{};
  _vkDepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  _vkStencilAttachment = VkRenderingAttachmentInfo{};
  _vkStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  if(attachment == nullptr) return;
  MT_ASSERT(attachment->target != nullptr);

  bool enableDepth =
          attachment->target->slice().aspectMask() & VK_IMAGE_ASPECT_DEPTH_BIT;
  bool enableStencil =
        attachment->target->slice().aspectMask() & VK_IMAGE_ASPECT_STENCIL_BIT;
  MT_ASSERT(enableDepth || enableStencil);

  // Определяем, в каком лэйауте должен быть image
  VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  if(!enableDepth) layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
  if(!enableStencil) layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

  _depthStencilAttachment = *attachment;
  if(enableDepth) _vkDepthAttachment = _fillFrom( *attachment, layout);
  if(enableStencil) _vkStencilAttachment = _fillFrom(*attachment, layout);

  // Оформляем доступ к таргету
  if(attachment->target->image().isLayoutAutoControlEnabled())
  {
    ImageAccess access{};
    access.slices[0] = attachment->target->slice();
    access.layouts[0] = layout;
    access.memoryAccess[0] = MemoryAccess{
            .readStagesMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .readAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .writeStagesMask =  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .writeAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
    access.slicesCount = 1;
    if(!_imagesAccess.addImageAccess(attachment->target->image(), access))
    {
      MT_ASSERT(false && "Unable to add depth-stencil target access description");
    }
  }

  // Захватываем владение, чтобы таргеты не удалились раньше времени
  _lockedResources.push_back(RefCounterReference(attachment->target));
}

VkRenderingAttachmentInfo FrameBuffer::_fillFrom(
                                        const DepthStencilAttachmentInfo& info,
                                        VkImageLayout layout) const noexcept
{
  MT_ASSERT(info.target != nullptr);
  MT_ASSERT(info.target->extent().z == 1);
  MT_ASSERT(_extent == glm::uvec2(info.target->extent()));

  VkRenderingAttachmentInfo result = VkRenderingAttachmentInfo{};
  result.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  result.imageView = info.target->handle();
  result.imageLayout = layout;
  result.resolveMode = VK_RESOLVE_MODE_NONE;
  result.loadOp = info.loadOp;
  result.storeOp = info.storeOp;
  result.clearValue = VkClearValue{.depthStencil = info.clearValue};

  return result;
}
