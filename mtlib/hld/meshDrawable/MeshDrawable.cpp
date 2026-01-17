#include <hld/drawCommand/DrawCommandList.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>

using namespace mt;

MeshDrawable::MeshDrawable() :
  Drawable(COMMANDS_DRAW),
  _onAssetUpdatedSlot(*this, &MeshDrawable::onAssetUpdated)
{
}

MeshDrawable::~MeshDrawable() noexcept
{
  _disconnectFromAsset();
}

void MeshDrawable::_disconnectFromAsset() noexcept
{
  if(_asset != nullptr)
  {
    _asset->configurationUpdated.removeSlot(_onAssetUpdatedSlot);
    _asset = nullptr;
  }
}

void MeshDrawable::setAsset(const MeshAsset* newAsset)
{
  try
  {
    _disconnectFromAsset();

    _asset = newAsset;

    if(_asset != nullptr)
    {
      _asset->configurationUpdated.addSlot(_onAssetUpdatedSlot);
    }
  }
  catch(...)
  {
    _disconnectFromAsset();
    throw;
  }
}

void MeshDrawable::onAssetUpdated()
{
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
    commandList.createCommand<MeshDrawCommand>( *_asset->technique(),
                                                *pass.pass,
                                                _asset->vertexCount(),
                                                _asset->maxInstancesCount(),
                                                pass.commandGroup,
                                                pass.layer,
                                                0.0f);
  }
}
