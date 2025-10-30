#pragma once

#include <vkr/queue/CommandProducerCompute.h>

namespace mt
{
  //  Продюсер команд, реализующий функционал графической очереди команд
  //  Так же включает в себя функционал CommandProducerTransfer и
  //    CommandProducerCompute
  class CommandProducerGraphic : public CommandProducerCompute
  {
  public:
    CommandProducerGraphic(CommandPoolSet& poolSet);
    CommandProducerGraphic(const CommandProducerGraphic&) = delete;
    CommandProducerCompute& operator = (const CommandProducerGraphic&) = delete;
    virtual ~CommandProducerGraphic() noexcept = default;

    //  Обертка вокруг vkCmdBlitImage
    //  Копировать один кусок Image в другое место (возможно в этот же Image,
    //    возможно в другой). Возможно преобразование формата и разрешения.
    //    последним
    //  Ограничения см. в описании vkCmdBlitImage
    void blitImage( const Image& srcImage,
                    VkImageAspectFlags srcAspect,
                    uint32_t srcArrayIndex,
                    uint32_t srcMipLevel,
                    const glm::uvec3& srcOffset,
                    const glm::uvec3& srcExtent,
                    const Image& dstImage,
                    VkImageAspectFlags dstAspect,
                    uint32_t dstArrayIndex,
                    uint32_t dstMipLevel,
                    const glm::uvec3& dstOffset,
                    const glm::uvec3& dstExtent,
                    VkFilter filter);
  };
}
