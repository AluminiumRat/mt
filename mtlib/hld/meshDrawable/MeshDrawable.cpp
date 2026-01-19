#include <hld/drawCommand/DrawCommandList.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <hld/DrawPlan.h>
#include <hld/FrameContext.h>

using namespace mt;

MeshDrawable::MeshDrawable(const MeshAsset& asset) :
  Drawable(COMMANDS_DRAW),
  _onAssetUpdatedSlot(*this, &MeshDrawable::onAssetUpdated),
  _positionMatrix(1),
  _prevPositionMatrix(1),
  _bivecMatrix(1)
{
  asset.addUpdateConfigurationSlot(_onAssetUpdatedSlot).release();
  _asset = &asset;
}

MeshDrawable::~MeshDrawable() noexcept
{
  _asset->removeUpdateConfigurationSlot(_onAssetUpdatedSlot);
  _asset = nullptr;
}

void MeshDrawable::setPositionMatrix(const glm::mat3x4& newValue)
{
  _positionMatrix = newValue;
  _updateBivecMatrix();
}

void MeshDrawable::setPrevPositionMatrix(const glm::mat3x4& newValue)
{
  _prevPositionMatrix = newValue;
}

void MeshDrawable::_updateBivecMatrix() noexcept
{
  const UniformVariable* bivecMatrixUniform = _asset->bivecMatrixUniform();
  if(bivecMatrixUniform == nullptr || !bivecMatrixUniform->isActive()) return;

  _bivecMatrix = _positionMatrix;
  _bivecMatrix = glm::inverse(_bivecMatrix);
  _bivecMatrix = glm::transpose(_bivecMatrix);
}

void MeshDrawable::onAssetUpdated()
{
  _updateBivecMatrix();
}

void MeshDrawable::addToDrawPlan( DrawPlan& plan,
                                  FrameTypeIndex frameTypeIndex) const
{
  if(_asset == nullptr) return;
  const MeshAsset::StageIndices& stages =
                                        _asset->availableStages(frameTypeIndex);
  for(StageIndex stage : stages) plan.addDrawable(*this, stage);
}

void MeshDrawable::addToCommandList(DrawCommandList& commandList,
                                    const FrameContext& frameContext) const
{
  if(_asset == nullptr) return;
  if(_asset->technique() == nullptr) return;

  const MeshAsset::StagePasses& passes = _asset->passes(
                                                    frameContext.frameTypeIndex,
                                                    frameContext.stageIndex);
  for(const MeshAsset::PassInfo pass : passes)
  {
    commandList.createCommand<MeshDrawCommand>( *this,
                                                *pass.pass,
                                                _asset->vertexCount(),
                                                _asset->maxInstancesCount(),
                                                pass.commandGroup,
                                                pass.layer,
                                                0.0f);
  }
}
