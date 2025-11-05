#include <util/Assert.h>
#include <vkr/FrameBuffer.h>

using namespace mt;

FrameBuffer::FrameBuffer( std::span<AttachmentInfo> colorAttachments,
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
                                    std::span<AttachmentInfo> colorAttachments)
{
  for(const AttachmentInfo& attachmentInfo : colorAttachments)
  {
    _colorAttachments.push_back(attachmentInfo);
    _vkColorAttachments.emplace_back(
          _fillFrom(attachmentInfo, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    _lockedResources.push_back(RefCounterReference(attachmentInfo.target));
    if(attachmentInfo.resolveTarget != nullptr)
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

  _depthAttachment = *depthAttachment;
  _vkDepthAttachment = _fillFrom( *depthAttachment,
                                  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
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

  _stencilAttachment = *stencilAttachment;
  _vkStencilAttachment = _fillFrom( *stencilAttachment,
                                    VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);
  _lockedResources.push_back(RefCounterReference(stencilAttachment->target));
  if(stencilAttachment->resolveTarget != nullptr)
  {
    _lockedResources.push_back(
                        RefCounterReference(stencilAttachment->resolveTarget));
  }
}
