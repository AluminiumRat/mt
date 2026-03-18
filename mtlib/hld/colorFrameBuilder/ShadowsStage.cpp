#include <hld/colorFrameBuilder/ShadowsStage.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/FrameBuildContext.h>
#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ShadowsStage::ShadowsStage(Device& device) :
  _device(device),
  _resolveConfigurator(new TechniqueConfigurator(device, "ShadowsResolve")),
  _resolveTechnique(*_resolveConfigurator),
  _resolvePass(_resolveTechnique.getOrCreatePass("ResolvePass")),
  _tlasBinding(_resolveTechnique.getOrCreateResourceBinding("tlas"))
{
  if(device.features().rayQuery.rayQuery == VK_TRUE)
  {
    loadConfigurator(*_resolveConfigurator, "shadows/resolve.tch");
    _resolveConfigurator->rebuildConfiguration();
  }
}

void ShadowsStage::draw(CommandProducerGraphic& commandProducer,
                        const FrameBuildContext& frameContext)
{
  if(_resolveFrameBuffer == nullptr) _buildFrameBuffer();

  const TLAS* tlas = frameContext.drawScene->tlas();
  _tlasBinding.setTLAS(tlas);

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                *_resolveFrameBuffer);
  if(_resolveTechnique.isReady())
  {
    Technique::BindGraphic bind(_resolveTechnique,
                                _resolvePass,
                                commandProducer);
    MT_ASSERT(bind.isValid())
    commandProducer.draw(4);
  }

  renderPass.endPass();
}

void ShadowsStage::_buildFrameBuffer()
{
  MT_ASSERT(_shadowBuffer != nullptr);

  FrameBuffer::ColorAttachmentInfo colorAttachment = {
                    .target = _shadowBuffer.get(),
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};
  _resolveFrameBuffer = new FrameBuffer(std::span(&colorAttachment, 1),
                                        nullptr);
}
