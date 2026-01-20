#include <mutex>

#include <util/Assert.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/FrameBufferFormat.h>

using namespace mt;

static std::vector<FrameBufferFormat> formatsTable;
static std::mutex formatsTableMutex;

uint32_t FrameBufferFormat::_getFormatIndex() const
{
  std::lock_guard lock(formatsTableMutex);

  // Для начала ищем, создавался ли такой формат ранее
  for(uint32_t index = 0; index < formatsTable.size(); index++)
  {
    const FrameBufferFormat& format = formatsTable[index];
    if( _colorAttachments == format._colorAttachments &&
        _depthStencilAttachment == format._depthStencilAttachment &&
        _samplesCount == format._samplesCount)
    {
      return index;
    }
  }

  // Не нашли такой же формат, добавляем новый в таблицу
  formatsTable.push_back(*this);
  return uint32_t(formatsTable.size() - 1);
}

void FrameBufferFormat::fillPipelineCreateInfo() noexcept
{
  _pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  _pipelineCreateInfo.viewMask = 0;
  _pipelineCreateInfo.colorAttachmentCount = uint32_t(_colorAttachments.size());
  _pipelineCreateInfo.pColorAttachmentFormats = _colorAttachments.data();

  if(_depthStencilAttachment != VK_FORMAT_UNDEFINED)
  {
    const ImageFormatFeatures& depthStencilDesc =
                                      getFormatFeatures(_depthStencilAttachment);
    if(depthStencilDesc.hasDepth)
    {
      _pipelineCreateInfo.depthAttachmentFormat = _depthStencilAttachment;
    }
    if(depthStencilDesc.hasStencil)
    {
      _pipelineCreateInfo.stencilAttachmentFormat = _depthStencilAttachment;
    }
  }
}

FrameBufferFormat::FrameBufferFormat( std::span<VkFormat> colotAttachments,
                                      VkFormat depthStencilAttachment,
                                      VkSampleCountFlagBits samplesCount) :
  _colorAttachments(colotAttachments.begin(), colotAttachments.end()),
  _depthStencilAttachment(depthStencilAttachment),
  _samplesCount(samplesCount),
  _pipelineCreateInfo{},
  _formatIndex(0)
{
  MT_ASSERT(colotAttachments.size() <= maxColorAttachments);
  fillPipelineCreateInfo();
  _formatIndex = _getFormatIndex();
}
