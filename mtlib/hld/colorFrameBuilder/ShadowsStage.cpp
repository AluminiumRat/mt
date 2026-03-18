#include <hld/colorFrameBuilder/ShadowsStage.h>
#include <hld/drawScene/DrawScene.h>
#include <hld/FrameBuildContext.h>
#include <technique/TechniqueLoader.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>

using namespace mt;

ShadowsStage::ShadowsStage(Device& device) :
  _device(device),
  _rayQueryTechniqueConfigurator(new TechniqueConfigurator( device,
                                                            "RayQueryShadows")),
  _rayQueryTechnique(*_rayQueryTechniqueConfigurator),
  _rayQueryPass(_rayQueryTechnique.getOrCreatePass("ResolvePass")),
  _tlasBinding(_rayQueryTechnique.getOrCreateResourceBinding("tlas"))
{
  if(device.features().rayQuery.rayQuery == VK_TRUE)
  {
    loadConfigurator( *_rayQueryTechniqueConfigurator,
                      "shadows/rayQueryShadows.tch");
    _rayQueryTechniqueConfigurator->rebuildConfiguration();
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
  if(_rayQueryTechnique.isReady())
  {
    Technique::BindGraphic bind(_rayQueryTechnique,
                                _rayQueryPass,
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
