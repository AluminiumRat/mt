#include <util/Assert.h>
#include <vkr/FrameBuffer.h>

using namespace mt;

FrameBuffer::FrameBuffer( std::span<const AttachmentInfo> colorAttachments,
                          const AttachmentInfo* depthAttachment,
                          const AttachmentInfo* stencilAttachment)
{
  MT_ASSERT(!colorAttachments.empty() || depthAttachment != nullptr || stencilAttachment != nullptr);
  MT_ASSERT(colorAttachments.size() <= maxColorAttachments);

  _extent = !colorAttachments.empty() ?
                    colorAttachments[0].target->extent() :
                    depthAttachment != nullptr ?
                                          depthAttachment->target->extent() :
                                          stencilAttachment->target->extent();

  _fillColorAttachments(colorAttachments);
  _fillDepthAttachment(depthAttachment);
  _fillStencilAttachment(stencilAttachment);

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
  if(depthAttachment != nullptr)
  {
    _vkBindingInfo.pDepthAttachment = &_vkDepthAttachment;
  }
  if (stencilAttachment != nullptr)
  {
    _vkBindingInfo.pStencilAttachment = &_vkStencilAttachment;
  }
}

void FrameBuffer::_fillColorAttachments(
                              std::span<const AttachmentInfo> colorAttachments)
{
  for(const AttachmentInfo& attachmentInfo : colorAttachments)
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
      _imagesAccess.addImageAccess(attachmentInfo.target->image(), access);
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
      _imagesAccess.addImageAccess( attachmentInfo.resolveTarget->image(),
                                    access);
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
                                          const AttachmentInfo& info,
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
  result.clearValue = info.clearValue;

  return result;
}

void FrameBuffer::_fillDepthAttachment(const AttachmentInfo* depthAttachment)
{
  _vkDepthAttachment = VkRenderingAttachmentInfo{};
  _vkDepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  if(depthAttachment == nullptr) return;
  MT_ASSERT(depthAttachment->target != nullptr);

  _depthAttachment = *depthAttachment;
  _vkDepthAttachment = _fillFrom( *depthAttachment,
                                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  // Оформляем доступ к основному таргету
  if(depthAttachment->target->image().isLayoutAutoControlEnabled())
  {
    ImageAccess access{};
    access.slices[0] = depthAttachment->target->slice();
    access.layouts[0] = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    access.memoryAccess[0] = MemoryAccess{
            .readStagesMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .readAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .writeStagesMask =  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .writeAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
    access.slicesCount = 1;
    _imagesAccess.addImageAccess(depthAttachment->target->image(), access);
  }

  // Оформляем доступ к resolveTarget-у
  if( depthAttachment->resolveTarget != nullptr &&
      depthAttachment->resolveTarget->image().isLayoutAutoControlEnabled())
  {
    ImageAccess access{};
    access.slices[0] = depthAttachment->resolveTarget->slice();
    access.layouts[0] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    access.memoryAccess[0] = MemoryAccess{
              .readStagesMask = VK_PIPELINE_STAGE_NONE,
              .readAccessMask = VK_ACCESS_NONE,
              .writeStagesMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .writeAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    access.slicesCount = 1;
    _imagesAccess.addImageAccess( depthAttachment->resolveTarget->image(),
                                  access);
  }

  // Захватываем владение, чтобы таргеты не удалились раньше времени
  _lockedResources.push_back(RefCounterReference(depthAttachment->target));
  if(depthAttachment->resolveTarget != nullptr)
  {
    _lockedResources.push_back(
                          RefCounterReference(depthAttachment->resolveTarget));
  }
}

void FrameBuffer::_fillStencilAttachment(
                                      const AttachmentInfo* stencilAttachment)
{
  _vkStencilAttachment = VkRenderingAttachmentInfo{};
  _vkStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  if(stencilAttachment == nullptr) return;
  MT_ASSERT(stencilAttachment->target != nullptr);

  _stencilAttachment = *stencilAttachment;
  _vkStencilAttachment = _fillFrom( *stencilAttachment,
                                    VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);

  // Оформляем доступ к основному таргету
  if(stencilAttachment->target->image().isLayoutAutoControlEnabled())
  {
    ImageAccess access{};
    access.slices[0] = stencilAttachment->target->slice();
    access.layouts[0] = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    access.memoryAccess[0] = MemoryAccess{
            .readStagesMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                              VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .readAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .writeStagesMask =  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .writeAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
    access.slicesCount = 1;
    _imagesAccess.addImageAccess(stencilAttachment->target->image(), access);
  }

  // Оформляем доступ к resolveTarget-у
  if( stencilAttachment->resolveTarget != nullptr &&
      stencilAttachment->resolveTarget->image().isLayoutAutoControlEnabled())
  {
    ImageAccess access{};
    access.slices[0] = stencilAttachment->resolveTarget->slice();
    access.layouts[0] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    access.memoryAccess[0] = MemoryAccess{
              .readStagesMask = VK_PIPELINE_STAGE_NONE,
              .readAccessMask = VK_ACCESS_NONE,
              .writeStagesMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              .writeAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    access.slicesCount = 1;
    _imagesAccess.addImageAccess( stencilAttachment->resolveTarget->image(),
                                  access);
  }

  // Захватываем владение, чтобы таргеты не удалились раньше времени
  _lockedResources.push_back(RefCounterReference(stencilAttachment->target));
  if(stencilAttachment->resolveTarget != nullptr)
  {
    _lockedResources.push_back(
                        RefCounterReference(stencilAttachment->resolveTarget));
  }
}
