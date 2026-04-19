#include <hld/colorFrameBuilder/HdrReprojector.h>
#include <vkr/queue/CommandProducerCompute.h>

using namespace mt;

HdrReprojector::HdrReprojector(Device& device) :
  _technique(device, "ssr/hdrReprojector.tch"),
  _reprojectPass(_technique.getOrCreatePass("ReprojectPass")),
  _prevHdrBinding(_technique.getOrCreateResourceBinding("prevHDR")),
  _reprojectedHdrBinding(
                    _technique.getOrCreateResourceBinding("reprojectedHdrOut")),
  _targetSizeUniform(_technique.getOrCreateUniform("params.targetSize")),
  _targetMipCountUniform(_technique.getOrCreateUniform("params.targetMipCount")),
  _baseMipSelection(_technique.getOrCreateSelection("BASE_MIP")),
  _gridSize(1)
{
}

void HdrReprojector::reproject(CommandProducerCompute& producer)
{
  MT_ASSERT(_reprojectedHdr != nullptr);

  glm::uvec2 currentGridSize = _gridSize;

  {
    _baseMipSelection.setValue("0");
    Technique::BindCompute bind(_technique, _reprojectPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }

  if(_reprojectedHdr->mipmapCount() > 5)
  {
    producer.imageBarrier(*_reprojectedHdr,
                          ImageSlice(*_reprojectedHdr),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

    _baseMipSelection.setValue("5");
    currentGridSize.x = std::max(currentGridSize.x / 16u, 1u);
    currentGridSize.y = std::max(currentGridSize.y / 16u, 1u);

    Technique::BindCompute bind(_technique, _reprojectPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }

  if(_reprojectedHdr->mipmapCount() > 10)
  {
    producer.imageBarrier(*_reprojectedHdr,
                          ImageSlice(*_reprojectedHdr),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

    _baseMipSelection.setValue("10");
    currentGridSize.x = std::max(currentGridSize.x / 16u, 1u);
    currentGridSize.y = std::max(currentGridSize.y / 16u, 1u);

    Technique::BindCompute bind(_technique, _reprojectPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }
}