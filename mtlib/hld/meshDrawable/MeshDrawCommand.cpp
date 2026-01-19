#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <technique/Technique.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

void MeshDrawCommand::draw( CommandProducerGraphic& producer,
                            std::span<const CommandPtr> commands)
{
  const Technique* technique = _drawable.asset().technique();
  if(technique == nullptr || !technique->isReady()) return;

  size_t commandProcessed = 0;
  while(commandProcessed != commands.size())
  {
    size_t chunkSize = commands.size() - commandProcessed;
    chunkSize = std::min(chunkSize, _maxInstances);
    _processChunk(producer,
                  *technique,
                  commands.subspan(commandProcessed, chunkSize));
    commandProcessed += chunkSize;
  }
}

void MeshDrawCommand::_processChunk(CommandProducerGraphic& producer,
                                    const Technique& technique,
                                    std::span<const CommandPtr> commands)
{
  TechniqueVolatileContext volatileContext =
                                      technique.createVolatileContext(producer);

  updateInstanceData(volatileContext, commands);

  Technique::Bind bind(technique, _pass, producer, &volatileContext);
  if (!bind.isValid()) return;

  producer.draw(_vertexCount, (uint32_t)commands.size());
}

void MeshDrawCommand::updateInstanceData(
                                    TechniqueVolatileContext& volatileContext,
                                    std::span<const CommandPtr> commands)
{
  _updatePositionMatrix(volatileContext, commands);
  _updateBivecMatrix(volatileContext, commands);
  _updatePrevPositionMatrix(volatileContext, commands);
}

void MeshDrawCommand::_updatePositionMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  const UniformVariable* matrixUniform =
                                      _drawable.asset().positionMatrixUniform();
  if(matrixUniform == nullptr || !matrixUniform->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.positionMatrix();
  }

  matrixUniform->setValue(volatileContext,
                          UniformVariable::ValueRef{.data = positionMatrices,
                                                    .dataSize = dataSize});
}

void MeshDrawCommand::_updatePrevPositionMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  const UniformVariable* matrixUniform =
                                  _drawable.asset().prevPositionMatrixUniform();
  if(matrixUniform == nullptr || !matrixUniform->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.prevPositionMatrix();
  }

  matrixUniform->setValue(volatileContext,
                          UniformVariable::ValueRef{.data = positionMatrices,
                                                    .dataSize = dataSize});
}

void MeshDrawCommand::_updateBivecMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  const UniformVariable* matrixUniform =
                                      _drawable.asset().bivecMatrixUniform();
  if (matrixUniform == nullptr || !matrixUniform->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat3x4);
  glm::mat3x4* bivecMatrices = (glm::mat3x4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    bivecMatrices[i] = meshCommand._drawable.bivecMatrix();
  }

  matrixUniform->setValue(volatileContext,
                          UniformVariable::ValueRef{.data = bivecMatrices,
                                                    .dataSize = dataSize});
}