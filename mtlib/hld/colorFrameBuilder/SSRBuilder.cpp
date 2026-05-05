#include <ImGui.h>

#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiRAII.h>
#include <hld/colorFrameBuilder/SSRBuilder.h>
#include <util/Assert.h>
#include <util/floorPow.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

SSRBuilder::SSRBuilder(Device& device) :
  _device(device),
  _hdrReprojector(device),
  _technique(device, "ssr/buildSSR.tch"),
  _marchingPass(_technique.getOrCreatePass("MarchingPass")),
  _debugPass(_technique.getOrCreatePass("DebugPass")),
  _reflectionBufferBinding(
    _technique.getOrCreateResourceBinding("outReflectionBuffer")),
  _prevHDRBufferBinding(_technique.getOrCreateResourceBinding("prevHDR")),
  _gridSize(1),
  _enableDebugDraw(false)
{
}

void SSRBuilder::buildReflection(CommandProducerGraphic& commandProducer)
{
  MT_ASSERT(_reflectionBuffer != nullptr);
  MT_ASSERT(_prevHDRBuffer != nullptr);

  if(_reprojectedHdr == nullptr) _createBuffers(commandProducer);

  _hdrReprojector.make(commandProducer);

  {
    Technique::BindCompute bind(_technique, _marchingPass, commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.dispatch(_gridSize);
  }
}

void SSRBuilder::_createBuffers(CommandProducerGraphic& commandProducer)
{
  glm::uvec2 reprojectedHDRSize = floorPow(_prevHDRBuffer->extent());
  uint32_t reprojectedHDRMipCount = Image::calculateMipNumber(reprojectedHDRSize);
  if(reprojectedHDRMipCount > 4) reprojectedHDRMipCount -= 4;
  reprojectedHDRMipCount = std::min(reprojectedHDRMipCount, 10u);
  _reprojectedHdr = new Image(_device,
                              VK_IMAGE_TYPE_2D,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT,
                              0,
                              VK_FORMAT_B10G11R11_UFLOAT_PACK32,
                              glm::uvec3(reprojectedHDRSize, 1),
                              VK_SAMPLE_COUNT_1_BIT,
                              1,
                              reprojectedHDRMipCount,
                              false,
                              "ReprojectedHDR");
  commandProducer.initLayout( *_reprojectedHdr, VK_IMAGE_LAYOUT_GENERAL);

  _hdrReprojector.setBuffers(*_reprojectedHdr, *_prevHDRBuffer);
}

void SSRBuilder::debugRender( FrameBuffer& target,
                              const Region& drawRegion,
                              CommandProducerGraphic& commandProducer)
{
  if(!_enableDebugDraw) return;

  MT_ASSERT(_reflectionBuffer != nullptr);
  MT_ASSERT(_prevHDRBuffer != nullptr);
  MT_ASSERT(_reprojectedHdr != nullptr);

  CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                target,
                                                drawRegion,
                                                drawRegion);
  {
    Technique::BindGraphic bind(_technique, _debugPass, commandProducer);
    MT_ASSERT(bind.isValid());
    commandProducer.draw(4);
  }
  renderPass.endPass();
}

void SSRBuilder::makeGui()
{
  ImGuiPropertyGrid grid("SSR");
  grid.addRow("Debug draw");
  ImGui::Checkbox("##DebugDraw", &_enableDebugDraw);

  if(_prevHDRBuffer != nullptr)
  {
    ImVec2 mousePos = ImGui::GetMousePos();
    _mousePosition = glm::vec2( mousePos.x / _prevHDRBuffer->extent().x,
                                mousePos.y / _prevHDRBuffer->extent().y);

    grid.addRow("Mouse position");
    ImGui::Text("(%.3f, %.3f)", _mousePosition.x, _mousePosition.y);
  }
}