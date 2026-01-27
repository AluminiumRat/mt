#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <technique/Technique.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

void MeshDrawCommand::draw( CommandProducerGraphic& producer,
                            std::span<const CommandPtr> commands)
{
  if(!_drawInfo.technique->isReady()) return;

  size_t commandProcessed = 0;
  while(commandProcessed != commands.size())
  {
    size_t chunkSize = commands.size() - commandProcessed;
    chunkSize = std::min(chunkSize, _maxInstances);
    _processChunk(producer,
                  commands.subspan(commandProcessed, chunkSize));
    commandProcessed += chunkSize;
  }
}

void MeshDrawCommand::_processChunk(CommandProducerGraphic& producer,
                                    std::span<const CommandPtr> commands)
{
  TechniqueVolatileContext volatileContext =
                          _drawInfo.technique->createVolatileContext(producer);

  updateInstanceData(volatileContext, commands);

  Technique::Bind bind( *_drawInfo.technique,
                        *_drawInfo.pass,
                        producer,
                        &volatileContext);
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
  if(!_drawInfo.positionMatrix->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.positionMatrix();
  }

  _drawInfo.positionMatrix->setValue( volatileContext,
                                      UniformVariable::ValueRef{
                                                      .data = positionMatrices,
                                                      .dataSize = dataSize});
}

void MeshDrawCommand::_updatePrevPositionMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  if(!_drawInfo.prevPositionMatrix->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.prevPositionMatrix();
  }

  _drawInfo.prevPositionMatrix->setValue( volatileContext,
                                          UniformVariable::ValueRef{
                                                      .data = positionMatrices,
                                                      .dataSize = dataSize});
}

void MeshDrawCommand::_updateBivecMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  if (!_drawInfo.bivecMatrix->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat3x4);
  glm::mat3x4* bivecMatrices = (glm::mat3x4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    bivecMatrices[i] = meshCommand._drawable.bivecMatrix();
  }

  _drawInfo.bivecMatrix->setValue(volatileContext,
                                  UniformVariable::ValueRef{
                                                        .data = bivecMatrices,
                                                        .dataSize = dataSize});
}