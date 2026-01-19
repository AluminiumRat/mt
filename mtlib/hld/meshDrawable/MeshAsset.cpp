#include <algorithm>
#include <stdexcept>

#include <hld/drawCommand/DrawCommand.h>
#include <hld/meshDrawable/MeshAsset.h>
#include <hld/HLDLib.h>

using namespace mt;

MeshAsset::MeshAsset(const char* debugName) :
  _debugName(debugName),
  _vertexCount(0),
  _maxInstancesCount(1),
  _positionMatrixUniform(nullptr),
  _prevPositionMatrixUniform(nullptr),
  _bivecMatrixUniform(nullptr)
{
}

void MeshAsset::_clearConfiguration() noexcept
{
  _positionMatrixUniform = nullptr;
  _prevPositionMatrixUniform = nullptr;
  _bivecMatrixUniform = nullptr;
  _technique.reset();
  _vertexCount = 0;
  _maxInstancesCount = 1;
  _availableStages.clear();
  _drawMap.clear();
}

void MeshAsset::setConfiguration(const Configuration& configuration)
{
  try
  {
    _clearConfiguration();

    _technique = configuration.technique;

    if(_technique != nullptr)
    {
      _positionMatrixUniform =
                &_technique->getOrCreateUniform("transformData.positionMatrix");
      _prevPositionMatrixUniform =
            &_technique->getOrCreateUniform("transformData.prevPositionMatrix");
      _bivecMatrixUniform =
                  &_technique->getOrCreateUniform("transformData.bivecMatrix");

      _vertexCount = configuration.vertexCount;
      _maxInstancesCount = configuration.maxInstancesCount;
      if(_maxInstancesCount == 0) throw std::runtime_error(_debugName + ": maxInstancesCount is 0");

      for(const PassConfig& passConfig : configuration.passes)
      {
        FrameTypeIndex frameTypeIndex =
                HLDLib::instance().getFrameTypeIndex(passConfig.frameTypeName);
        StageIndex stageIndex =
                        HLDLib::instance().getStageIndex(passConfig.stageName);
        TechniquePass& pass =
                      _technique->getOrCreatePass(passConfig.passName.c_str());
        _addPass(frameTypeIndex, stageIndex, passConfig.layer, pass);
      }
    }

    _configurationUpdated.emit();
  }
  catch(...)
  {
    _clearConfiguration();
    _configurationUpdated.emit();
    throw;
  }
}

void MeshAsset::_addPass( FrameTypeIndex frameTypeIndex,
                          StageIndex stageIndex,
                          int32_t layer,
                          TechniquePass& pass)
{
  _addAvailableStage(frameTypeIndex, stageIndex);

  if(_drawMap.size() <= frameTypeIndex) _drawMap.resize(frameTypeIndex + 1);
  FrameStages& stages = _drawMap[frameTypeIndex];

  if(stages.size() <= stageIndex) stages.resize(stageIndex + 1);
  StagePasses& passes = stages[stageIndex];

  PassInfo newPass{};
  newPass.layer = layer;
  newPass.pass = &pass;
  if(_maxInstancesCount > 1)
  {
    newPass.commandGroup = HLDLib::instance().allocateDrawCommandGroup();
  }
  else newPass.commandGroup = DrawCommand::noGroup;

  passes.push_back(newPass);
}

void MeshAsset::_addAvailableStage( FrameTypeIndex frameTypeIndex,
                                    StageIndex stageIndex)
{
  if(_availableStages.size() <= frameTypeIndex)
  {
    _availableStages.resize(frameTypeIndex + 1);
  }

  StageIndices& stages = _availableStages[frameTypeIndex];
  if(std::find( stages.begin(),
                stages.end(),
                stageIndex) == stages.end())
  {
    stages.push_back(stageIndex);
  }
}
