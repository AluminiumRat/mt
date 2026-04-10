#include <hld/colorFrameBuilder/HiZBuilder.h>
#include <vkr/queue/CommandProducerCompute.h>
#include <vkr/DataBuffer.h>
#include <vkr/Device.h>

using namespace mt;

HiZBuilder::HiZBuilder(Device& device) :
  _technique(device, "hiZ/buildHiZ.tch"),
  _buildPass(_technique.getOrCreatePass("BuildPass")),
  _hiZBinding(_technique.getOrCreateResourceBinding("hiZ")),
  _hizSizeUniform(_technique.getOrCreateUniform("params.hiZSize")),
  _hizMipCountUniform(_technique.getOrCreateUniform("params.hiZMipCount")),
  _baseMipSelection(_technique.getOrCreateSelection("BASE_MIP")),
  _gridSize(1)
{
}

void HiZBuilder::buildHiZ(CommandProducerCompute& producer)
{
  MT_ASSERT(_hiZ != nullptr);

  glm::uvec2 currentGridSize = _gridSize;

  {
    _baseMipSelection.setValue("0");
    Technique::BindCompute bind(_technique, _buildPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }

  if(_hiZ->mipmapCount() > 5)
  {
    producer.imageBarrier(*_hiZ,
                          ImageSlice(*_hiZ),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

    _baseMipSelection.setValue("5");
    currentGridSize.x = std::max(currentGridSize.x / 16u, 1u);
    currentGridSize.y = std::max(currentGridSize.y / 16u, 1u);

    Technique::BindCompute bind(_technique, _buildPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }

  if(_hiZ->mipmapCount() > 10)
  {
    producer.imageBarrier(*_hiZ,
                          ImageSlice(*_hiZ),
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_READ_BIT);

    _baseMipSelection.setValue("10");
    currentGridSize.x = std::max(currentGridSize.x / 16u, 1u);
    currentGridSize.y = std::max(currentGridSize.y / 16u, 1u);

    Technique::BindCompute bind(_technique, _buildPass, producer);
    MT_ASSERT(bind.isValid());
    producer.dispatch(currentGridSize);
  }
}
