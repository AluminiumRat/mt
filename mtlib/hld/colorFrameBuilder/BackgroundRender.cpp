#include <stdexcept>

#include <hld/colorFrameBuilder/BackgroundRender.h>
#include <resourceManagement/TechniqueManager.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

BackgroundRender::BackgroundRender( Device& device,
                                    TechniqueManager& techniqueManager) :
  _device(device),
  _envmapTechnique(techniqueManager.loadImmediately("background/envmap.tch",
                                                    _device)),
  _envmapPass(_envmapTechnique->getOrCreatePass("RenderPass"))
{
  if(_envmapTechnique->configuration() == nullptr) throw std::runtime_error("BackgroundRender: unable to load technique 'background/envmap.tch'");
}

void BackgroundRender::_buildFrameBuffer()
{
  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = _hdrBuffer.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

  FrameBuffer::DepthStencilAttachmentInfo depthAttachment = {
                                        .target = _depthBuffer.get(),
                                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

  _frameBuffer = new FrameBuffer( std::span(&colorAttachment, 1),
                                  &depthAttachment);}

void BackgroundRender::draw(CommandProducerGraphic& commandProducer,
                            glm::uvec2 viewport)
{
  MT_ASSERT(_hdrBuffer != nullptr);
  MT_ASSERT(_depthBuffer != nullptr);
  MT_ASSERT(viewport.x != 0 && viewport.y != 0);

  if(_frameBuffer == nullptr) _buildFrameBuffer();

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                *_frameBuffer,
                                                std::nullopt,
                                                viewport);
    Technique::BindGraphic bind(*_envmapTechnique,
                                _envmapPass,
                                commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.draw(4);
    bind.release();
  renderPass.endPass();
}
