#include <hld/colorFrameBuilder/ReprojectionBufferUpdater.h>
#include <technique/TechniqueLoader.h>
#include <util/Assert.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

ReprojectionBufferUpdater::ReprojectionBufferUpdater(Device& device) :
  _device(device),
  _techniqueConfigurator(new TechniqueConfigurator(
                                                  device,
                                                  "ReprojectionBufferUpdate")),
  _technique(*_techniqueConfigurator),
  _updatePass(_technique.getOrCreatePass("UpdatePass")),
  _copyHistoryPass(_technique.getOrCreatePass("CopyDepthHistoryPass")),
  _historyAvailableSelection(
                          _technique.getOrCreateSelection("HISTORY_AVAILABLE")),
  _reprojectionBufferBinding(
                _technique.getOrCreateResourceBinding("outReprojectionBuffer")),
  _depthHistoryBinding(_technique.getOrCreateResourceBinding("depthHistory")),
  _gridSize(1)
{
  loadConfigurator( *_techniqueConfigurator,
                    "reprojectionBuffer/updateReprojectionBuffer.tch");
  _techniqueConfigurator->rebuildConfiguration();
}

void ReprojectionBufferUpdater::updateReprojection(
                                        CommandProducerGraphic& commandProducer)
{
  if(_depthHistory == nullptr)
  {
    createBuffers(commandProducer);
    _historyAvailableSelection.setValue("0");
  }
  else _historyAvailableSelection.setValue("1");

  {
    Technique::BindCompute bind(_technique, _updatePass, commandProducer);
    MT_ASSERT(bind.isValid())
    commandProducer.dispatch(_gridSize);
  }

  commandProducer.pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

  {
    Technique::BindCompute bind(_technique, _copyHistoryPass, commandProducer);
    MT_ASSERT(bind.isValid())
    commandProducer.dispatch(_gridSize);
  }
}

void ReprojectionBufferUpdater::createBuffers(
                                        CommandProducerGraphic& commandProducer)
{
  glm::uvec3 extent = _reprojectionBufferBinding.image()->extent();
  Ref<Image> depthHistoryImage(new Image(
                                    _device,
                                    VK_IMAGE_TYPE_2D,
                                    VK_IMAGE_USAGE_STORAGE_BIT,
                                    0,
                                    VK_FORMAT_R16_SFLOAT,
                                    extent,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    1,
                                    1,
                                    false,
                                    "ReprojectionBufferUpdater::DepthHistory"));
  commandProducer.initLayout(*depthHistoryImage, VK_IMAGE_LAYOUT_GENERAL);
  _depthHistory = new ImageView(*depthHistoryImage);
  _depthHistoryBinding.setImage(_depthHistory);
}
