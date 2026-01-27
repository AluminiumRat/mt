#include <hld/drawCommand/DrawCommandList.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <hld/DrawPlan.h>

using namespace mt;

MeshDrawable::MeshDrawable(const MeshAsset& asset) :
  Drawable3D(COMMANDS_DRAW),
  _onAssetUpdatedSlot(*this, &MeshDrawable::onAssetUpdated),
  _positionMatrix(1),
  _prevPositionMatrix(1),
  _bivecMatrix(1)
{
  asset.techniquesChanged.addSlot(_onAssetUpdatedSlot);
  asset.boundChanged.addSlot(_onAssetUpdatedSlot);
  _asset = &asset;
}

MeshDrawable::~MeshDrawable() noexcept
{
  _asset->techniquesChanged.removeSlot(_onAssetUpdatedSlot);
  _asset->boundChanged.removeSlot(_onAssetUpdatedSlot);
}

void MeshDrawable::setPositionMatrix(const glm::mat4& newValue)
{
  _positionMatrix = newValue;
  _updateBivecMatrix();
  _updateBoundingBox();
}

void MeshDrawable::setPrevPositionMatrix(const glm::mat4& newValue)
{
  _prevPositionMatrix = newValue;
}

void MeshDrawable::_updateBivecMatrix() noexcept
{
  if(!_asset->usesBivecMatrix()) return;

  _bivecMatrix = _positionMatrix;
  _bivecMatrix = glm::inverse(_bivecMatrix);
  _bivecMatrix = glm::transpose(_bivecMatrix);
}

void MeshDrawable::_updateBoundingBox() noexcept
{
  setBoundingBox(_asset->bound().translated(_positionMatrix));
}

void MeshDrawable::onAssetUpdated()
{
  _updateBivecMatrix();
  _updateBoundingBox();
}

void MeshDrawable::addToDrawPlan( DrawPlan& plan,
                                  FrameTypeIndex frameTypeIndex) const
{
  const MeshAsset::StageIndices& stages =
                                        _asset->availableStages(frameTypeIndex);
  for(StageIndex stage : stages) plan.addDrawable(*this, stage);
}

void MeshDrawable::addToCommandList(DrawCommandList& commandList,
                                    FrameTypeIndex frame,
                                    StageIndex stage,
                                    const void* extraData) const
{
  const MeshAsset::StagePasses& passes = _asset->passes(frame, stage);
  for(const MeshAsset::PassInfo pass : passes)
  {
    commandList.createCommand<MeshDrawCommand>( *this,
                                                *pass.technique,
                                                *pass.pass,
                                                *pass.positionMatrix,
                                                *pass.prevPositionMatrix,
                                                *pass.bivecMatrix,
                                                _asset->vertexCount(),
                                                _asset->maxInstancesCount(),
                                                pass.commandGroup,
                                                pass.layer,
                                                0.0f);
  }
}
