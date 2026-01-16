#include <hld/drawCommand/DrawCommandList.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>

using namespace mt;

MeshDrawable::MeshDrawable() :
  Drawable(COMMANDS_DRAW)
{
}

void MeshDrawable::setAsset(MeshAsset* newAsset)
{
  _asset = newAsset;
}

void MeshDrawable::addToDrawPlan( DrawPlan& plan,
                                  uint32_t frameTypeIndex) const
{
  if(_asset == nullptr) return;
  const MeshAsset::StageIndices& stages =
                                        _asset->availableStages(frameTypeIndex);
  for(uint32_t stage : stages) plan.addDrawable(*this, stage);
}

void MeshDrawable::addToCommandList(DrawCommandList& commandList,
                                    const FrameContext& frameContext) const
{
  if(_asset == nullptr) return;
  if(_asset->technique() == nullptr) return;

  const MeshAsset::StagePasses& passes = _asset->passes(
                                                  frameContext.frameTypeIndex,
                                                  frameContext.drawStageIndex);
  for(const MeshAsset::PassInfo pass : passes)
  {
    uint32_t vertexCount = _asset->vertexCount();
    uint32_t maxInstances = _asset->maxInstancesCount();
    float distance = 0;
    commandList.createCommand<MeshDrawCommand>( *_asset->technique(),
                                                *pass.pass,
                                                vertexCount,
                                                maxInstances,
                                                pass.commandGroup,
                                                pass.layer,
                                                distance);
  }
}
