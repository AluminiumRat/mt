#pragma once

#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  //  Описание фрэймбуфера без привязки к конкретным ImageView
  //  Необходим при создании пайплайнов а так же для проверки, что
  //    пайплайн совместим с фрэймбуфером.
  //  Основное назначение - заранее просчитать нужную информацию, чтобы
  //    обеспечить быстрое сравнение форматов.
  class FrameBufferFormat
  {
  public:
    //  Максимальное количество одновременно подключаемых колор таргетов
    static constexpr uint32_t maxColorAttachments = 8;

  public:
    FrameBufferFormat(std::span<VkFormat> colotAttachments,
                      VkFormat depthStencilAttachment,
                      VkSampleCountFlagBits samplesCount);
    FrameBufferFormat(const FrameBufferFormat&) = default;
    FrameBufferFormat& operator = (const FrameBufferFormat&) = default;
    ~FrameBufferFormat() noexcept = default;

    inline bool operator == (const FrameBufferFormat& other) const noexcept;

    inline const VkPipelineRenderingCreateInfo&
                                            pipelineCreateInfo() const noexcept;

    inline const std::vector<VkFormat>& colorAttachments() const noexcept;
    inline VkFormat depthStencilAttachment() const noexcept;

    inline VkSampleCountFlagBits samplesCount() const noexcept;

  private:
    void fillPipelineCreateInfo() noexcept;
    uint32_t _getFormatIndex() const;

  private:
    std::vector<VkFormat> _colorAttachments;
    VkFormat _depthStencilAttachment;
    VkSampleCountFlagBits _samplesCount;
    VkPipelineRenderingCreateInfo _pipelineCreateInfo;
    uint32_t _formatIndex;
  };

  inline bool FrameBufferFormat::operator == (
                                  const FrameBufferFormat& other) const noexcept
  {
    return _formatIndex == other._formatIndex;
  }

  inline const VkPipelineRenderingCreateInfo&
                          FrameBufferFormat::pipelineCreateInfo() const noexcept
  {
    return _pipelineCreateInfo;
  }

  inline const std::vector<VkFormat>&
                            FrameBufferFormat::colorAttachments() const noexcept
  {
    return _colorAttachments;
  }

  inline VkFormat FrameBufferFormat::depthStencilAttachment() const noexcept
  {
    return _depthStencilAttachment;
  }

  inline VkSampleCountFlagBits FrameBufferFormat::samplesCount() const noexcept
  {
    return _samplesCount;
  }
}