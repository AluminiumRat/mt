#include <hld/drawCommand/DrawCommandList.h>
#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <hld/DrawPlan.h>
#include <hld/FrameBuildContext.h>
#include <util/Camera.h>

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

void MeshDrawable::_updateBivecMatrix() noexcept
{
  _bivecMatrix = _positionMatrix;
  _bivecMatrix = glm::inverse(_bivecMatrix);
  _bivecMatrix = glm::transpose(_bivecMatrix);
}

void MeshDrawable::setPrevPositionMatrix(const glm::mat4& newValue)
{
  _prevPositionMatrix = newValue;
}

void MeshDrawable::_updateBoundingBox() noexcept
{
  setBoundingBox(_asset->bound().translated(_positionMatrix));
}

void MeshDrawable::onAssetUpdated()
{
  _updateBoundingBox();
}

void MeshDrawable::addToDrawPlan( DrawPlan& plan,
                                  const FrameBuildContext& frameContext) const
{
  const MeshAsset::StageIndices& stages =
                              _asset->availableStages(frameContext.frameType);
  for(StageIndex stage : stages) plan.addDrawable(*this, stage);
}

void MeshDrawable::addToCommandList(DrawCommandList& commandList,
                                    const FrameBuildContext& frameContext,
                                    StageIndex stage,
                                    const void* extraData) const
{
  const MeshAsset::StagePasses& passes = _asset->passes(frameContext.frameType,
                                                        stage);
  if(passes.empty()) return;

  glm::vec3 meshPosition = _positionMatrix[3];
  glm::vec3 cameraToMesh = meshPosition - frameContext.viewCamera->eyePoint();
  float distance = glm::dot(frameContext.viewCamera->frontVector(),
                            cameraToMesh);

  for(const MeshDrawInfo& drawInfo : passes)
  {
    commandList.createCommand<MeshDrawCommand>( *this,
                                                drawInfo,
                                                _asset->vertexCount(),
                                                distance);
  }
}
