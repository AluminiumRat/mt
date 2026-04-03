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
  _transformMatrix(1),
  _prevTransformMatrix(1),
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

void MeshDrawable::setTransformMatrix(const glm::mat4& newValue)
{
  _transformMatrix = newValue;
  _updateBivecMatrix();
  _updateBoundingBox();
}

void MeshDrawable::_updateBivecMatrix() noexcept
{
  _bivecMatrix = _transformMatrix;
  _bivecMatrix = glm::inverse(_bivecMatrix);
  _bivecMatrix = glm::transpose(_bivecMatrix);
}

void MeshDrawable::setPrevTransformMatrix(const glm::mat4& newValue)
{
  _prevTransformMatrix = newValue;
}

void MeshDrawable::_updateBoundingBox() noexcept
{
  setBoundingBox(_asset->bound().translated(_transformMatrix));
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

  glm::vec3 meshPosition = _transformMatrix[3];
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
