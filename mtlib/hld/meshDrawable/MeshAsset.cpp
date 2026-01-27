#include <stdexcept>

#include <hld/meshDrawable/MeshAsset.h>
#include <hld/HLDLib.h>
#include <util/Abort.h>
#include <util/Log.h>

using namespace mt;

MeshAsset::MeshAsset(const char* debugName) :
  _debugName(debugName),
  _onTechniqueUpdatedSlot(*this, &MeshAsset::_rebuildDrawMap),
  _usesBivecMatrix(false),
  _usesPrevMatrix(false),
  _vertexCount(0),
  _bound(0,0,0,0,0,0)
{
}

MeshAsset::~MeshAsset()
{
  for(std::unique_ptr<Technique>& technique : _techniques)
  {
    technique->configurationUpdatedSignal.removeSlot(_onTechniqueUpdatedSlot);
  }
}

void MeshAsset::clear() noexcept
{
  _availableStages.clear();
  _drawMap.clear();
  _commonBuffers.clear();

  for (std::unique_ptr<Technique>& technique : _techniques)
  {
    technique->configurationUpdatedSignal.removeSlot(_onTechniqueUpdatedSlot);
  }
  _techniques.clear();

  _usesBivecMatrix = false;
  _usesPrevMatrix = false;
  _vertexCount = 0;
  _bound = AABB(0, 0, 0, 0, 0, 0);

  techniquesChanged.emit();
  boundChanged.emit();
}

void MeshAsset::addTechnique(std::unique_ptr<Technique> technique)
{
  std::string techniqueName = technique->debugName();

  try
  {
    Technique* techniquePtr = technique.get();
    _techniques.push_back(std::move(technique));

    techniquePtr->configurationUpdatedSignal.addSlot(_onTechniqueUpdatedSlot);

    for(BufferInfo& bufferInfo : _commonBuffers)
    {
      ResourceBinding& binding = 
              techniquePtr->getOrCreateResourceBinding(bufferInfo.name.c_str());
      binding.setBuffer(bufferInfo.buffer);
    }

    _rebuildDrawMap();
  }
  catch(std::exception& error)
  {
    Log::error() << _debugName << ": unable to add technique " << techniqueName << ": " << error.what();
    clear();
    throw error;
  }

  techniquesChanged.emit();
}

void MeshAsset::addTechniques(std::span<std::unique_ptr<Technique>> techniques)
{
  try
  {
    for(std::unique_ptr<Technique>& technique : techniques)
    {
      Technique* techniquePtr = technique.get();
      _techniques.push_back(std::move(technique));

      techniquePtr->configurationUpdatedSignal.addSlot(_onTechniqueUpdatedSlot);

      for(BufferInfo& bufferInfo : _commonBuffers)
      {
        ResourceBinding& binding = 
              techniquePtr->getOrCreateResourceBinding(bufferInfo.name.c_str());
        binding.setBuffer(bufferInfo.buffer);
      }
    }

    _rebuildDrawMap();
  }
  catch(std::exception& error)
  {
    Log::error() << _debugName << ": unable to add techniques: " << error.what();
    clear();
    throw error;
  }

  techniquesChanged.emit();
}

void MeshAsset::deleteTechnique(const Technique& technique) noexcept
{
  try
  {
    for(Techniques::iterator iTechnique = _techniques.begin();
        iTechnique != _techniques.end();
        iTechnique++)
    {
      if(iTechnique->get() == &technique)
      {
        (*iTechnique)->configurationUpdatedSignal.removeSlot(
                                                      _onTechniqueUpdatedSlot);
        _techniques.erase(iTechnique);
        break;
      }
    }

    _rebuildDrawMap();
  }
  catch (std::exception& error)
  {
    Log::error() << _debugName << ": unable to delete technique: " << error.what();
    Abort(error.what());
  }

  techniquesChanged.emit();
}

void MeshAsset::_rebuildDrawMap()
{
  try
  {
    _availableStages.clear();
    _drawMap.clear();
    _usesBivecMatrix = false;
    _usesPrevMatrix = false;

    for (std::unique_ptr<Technique>& technique : _techniques)
    {
      _updateDrawmapFromTechnique(*technique);
    }
  }
  catch(std::exception& error)
  {
    Log::error() << _debugName << ": unable to rebuild drawmap: " << error.what();
    clear();
    throw error;
  }
}

void MeshAsset::_updateDrawmapFromTechnique(Technique& technique)
{
  const TechniqueConfiguration* configuration = technique.configuration();
  if (configuration == nullptr) return;

  UniformVariable& positionMatrix =
                  technique.getOrCreateUniform("transformData.positionMatrix");

  UniformVariable& prevPositionMatrix =
              technique.getOrCreateUniform("transformData.prevPositionMatrix");
  _usesPrevMatrix |= prevPositionMatrix.isActive();

  UniformVariable& bivecMatrix =
                      technique.getOrCreateUniform("transformData.bivecMatrix");
  _usesBivecMatrix |= bivecMatrix.isActive();

  //  Обходим все проходы в технике и добавляем их в _drawMap
  for(const PassConfiguration& passConfig : configuration->_passes)
  {
    //  Если для прохода не указаны фрэйм и стадия, то мы не можем его
    //  использовать
    if(passConfig.frameType.empty() || passConfig.stageName.empty()) continue;

    FrameTypeIndex frameTypeIndex =
                    HLDLib::instance().getFrameTypeIndex(passConfig.frameType);
    StageIndex stageIndex =
                        HLDLib::instance().getStageIndex(passConfig.stageName);

    //  Заполняем информацию об новом проходе отрисовки
    MeshDrawInfo newPassInfo{};
    newPassInfo.layer = passConfig.layer;
    if (passConfig.maxInstances > 1)
    {
      newPassInfo.commandGroup = HLDLib::instance().allocateDrawCommandGroup();
    }
    else newPassInfo.commandGroup = DrawCommand::noGroup;
    newPassInfo.technique = &technique;
    newPassInfo.pass = &technique.getOrCreatePass(passConfig.name.c_str());
    newPassInfo.positionMatrix = &positionMatrix;
    newPassInfo.prevPositionMatrix = &prevPositionMatrix;
    newPassInfo.bivecMatrix = &bivecMatrix;
    newPassInfo.maxInstances = passConfig.maxInstances;

    //  Ищем место в _drawMap и вставляем туда новый проход отрисовки
    if (_drawMap.size() <= frameTypeIndex) _drawMap.resize(frameTypeIndex + 1);
    FrameStages& stages = _drawMap[frameTypeIndex];
    if (stages.size() <= stageIndex) stages.resize(stageIndex + 1);
    StagePasses& passes = stages[stageIndex];
    passes.push_back(newPassInfo);

    //  Делаем отметку о новой доступной стадии отрисовки
    _addAvailableStage(frameTypeIndex, stageIndex);
  }
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

void MeshAsset::setCommonBuffer(const char* bufferName,
                                const DataBuffer& buffer)
{
  try
  {
    //  Проверяем, есть ли уже буфер с таким именем, и если нет, то добавляем
    //  в список
    bool exists = false;
    for(BufferInfo& bufferInfo : _commonBuffers)
    {
      if(bufferInfo.name == bufferName)
      {
        exists = true;
        break;
      }
    }
    if(!exists) _commonBuffers.push_back(BufferInfo{
                                                  .name = bufferName,
                                                  .buffer = ConstRef(&buffer)});

    //  Обходим все техники и выставляем буфер
    for(std::unique_ptr<Technique>& technique : _techniques)
    {
      ResourceBinding& binding =
                              technique->getOrCreateResourceBinding(bufferName);
      binding.setBuffer(&buffer);
    }
  }
  catch(std::exception& error)
  {
    Log::error() << _debugName << ": unable to add buffer " << bufferName << ": " << error.what();
    clear();
    throw error;
  }
}
