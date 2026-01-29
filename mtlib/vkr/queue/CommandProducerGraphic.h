#pragma once

#include <array>

#include <vkr/image/ImageAccessMultiset.h>
#include <vkr/queue/CommandProducerCompute.h>

namespace mt
{
  class FrameBuffer;
  class DescriptorSet;
  class GraphicPipeline;
  class PipelineLayout;

  //  Продюсер команд, реализующий функционал графической очереди команд
  //  Так же включает в себя функционал CommandProducerTransfer и
  //    CommandProducerCompute
  class CommandProducerGraphic : public CommandProducerCompute
  {
  public:
    //  Максимальное количество дескриптер сетов, которые можно прибиндить к
    //  графическому пайплайну
    //  Нулевой слот ImageAccessMultiset используется для фрэймбуфера
    static constexpr uint32_t maxGraphicDescriptorSetsNumber =
                                          ImageAccessMultiset::maxSetsCount - 1;

  public:
    //  RAII оболочка вокруг биндинга фрэйм буфера.
    //  Указывает продюсеру, куда должны отрисовывать команды рендера.
    //  В конструкторе фрэйм буфер подключается к комманд продюсеру, происходит
    //    очистка рендер таргетов.
    //  В деструкторе сбрасываются все кэши таргетов, происходит резолв
    //    мультисэмплинга и фрэйм буфер отключается от продюсера.
    //  Одновременно на одном продюсере может происходить не более 1 паса.
    //  Если автоконтроль лэйаутов отключен для каких-либо из таргетов, то
    //    к моменту создания рендер пасса они должны быть переведены
    //    в лэйауты VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL или
    //    VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL в зависимости от их
    //    использования.
    class RenderPass
    {
    public:
      inline RenderPass(CommandProducerGraphic& commandProducer,
                        const FrameBuffer& frameBuffer);
      RenderPass(const RenderPass&) = delete;
      RenderPass& operator = (const RenderPass&) = delete;
      inline RenderPass(RenderPass&& other) noexcept;
      inline RenderPass& operator = (RenderPass&& other) noexcept;
      inline ~RenderPass() noexcept;

      //  Принудительное окончание рендер пасса. Делает то же самое, что и
      //  деструктор.
      inline void endPass() noexcept;

      inline const FrameBuffer& frameBuffer() const noexcept;

    private:
      CommandProducerGraphic* _commandProducer;
      const FrameBuffer* _frameBuffer;
    };

  public:
    //  Если debugName - не пустая строка, то все команды продюсера
    //  будут обернуты в vkCmdBeginDebugUtilsLabelEXT и
    //  vkCmdEndDebugUtilsLabelEXT
    CommandProducerGraphic(CommandPoolSet& poolSet, const char* debugName);
    CommandProducerGraphic(const CommandProducerGraphic&) = delete;
    CommandProducerCompute& operator = (const CommandProducerGraphic&) = delete;
    virtual ~CommandProducerGraphic() noexcept;

    inline const FrameBuffer* curentFrameBuffer() const noexcept;

    void setGraphicPipeline(const GraphicPipeline& pipeline);

    // Подключить набор ресурсов к графическому пайплайну
    void bindDescriptorSetGraphic(const DescriptorSet& descriptorSet,
                                  uint32_t setIndex,
                                  const PipelineLayout& layout);
    // Отключить набор ресурсов от пайплайна
    void unbindDescriptorSetGraphic(uint32_t setIndex) noexcept;

    //  Обертка вокруг vkCmdDraw
    //  Обычная отрисовка фиксированного количества вершин без индексного буфера
    void draw(uint32_t vertexCount,
              uint32_t instanceCount = 1,
              uint32_t firstVertex = 0,
              uint32_t firstInstance = 0);

    //  Обертка вокруг vkCmdBlitImage
    //  Копировать один кусок Image в другое место (возможно в этот же Image,
    //    возможно в другой). Возможно преобразование формата и разрешения.
    //    последним
    //  Ограничения см. в описании vkCmdBlitImage
    //  Может работать вне рендер пасса
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

  protected:
    virtual void finalizeCommands() noexcept;

  private:
    void _beginPass(RenderPass& renderPass);
    void _endPass() noexcept;

  private:
    RenderPass* _currentPass;
    ImageAccessMultiset _pipelineAccesses;
  };

  inline CommandProducerGraphic::RenderPass::RenderPass(
                                        CommandProducerGraphic& commandProducer,
                                        const FrameBuffer& frameBuffer) :
    _commandProducer(&commandProducer),
    _frameBuffer(&frameBuffer)
  {
    _commandProducer->_beginPass(*this);
  }

  inline CommandProducerGraphic::RenderPass::RenderPass(
                                                  RenderPass&& other) noexcept :
    _commandProducer(other._commandProducer),
    _frameBuffer(other._frameBuffer)
  {
    other._commandProducer = nullptr;
  }

  inline CommandProducerGraphic::RenderPass&
      CommandProducerGraphic::RenderPass::operator = (RenderPass&& other) noexcept
  {
    if(_commandProducer != nullptr) endPass();

    _commandProducer = other._commandProducer;
    _frameBuffer = other._frameBuffer;
    other._commandProducer = nullptr;
  }

  inline CommandProducerGraphic::RenderPass::~RenderPass() noexcept
  {
    if (_commandProducer != nullptr) endPass();
  }

  inline const FrameBuffer&
                CommandProducerGraphic::RenderPass::frameBuffer() const noexcept
  {
    return *_frameBuffer;
  }

  inline void CommandProducerGraphic::RenderPass::endPass() noexcept
  {
    MT_ASSERT(_commandProducer != nullptr);
    _commandProducer->_endPass();
    _commandProducer = nullptr;
  }

  inline const FrameBuffer*
                      CommandProducerGraphic::curentFrameBuffer() const noexcept
  {
    if(_currentPass == nullptr) return nullptr;
    return &_currentPass->frameBuffer();
  }
}
