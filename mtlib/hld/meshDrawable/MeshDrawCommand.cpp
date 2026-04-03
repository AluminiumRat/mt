#include <hld/meshDrawable/MeshDrawable.h>
#include <hld/meshDrawable/MeshDrawCommand.h>
#include <technique/Technique.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

void MeshDrawCommand::draw( CommandProducerGraphic& producer,
                            std::span<const CommandPtr> commands)
{
  //  Проверяем, что конфигурация уже прогрузилась
  if(_drawInfo.technique->configuration() == nullptr) return;

  size_t commandProcessed = 0;
  while(commandProcessed != commands.size())
  {
    size_t chunkSize = commands.size() - commandProcessed;
    chunkSize = std::min(chunkSize, (size_t)_drawInfo.maxInstances);
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

  Technique::BindGraphic bind(*_drawInfo.technique,
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
  _updateTransformMatrix(volatileContext, commands);
  _updateBivecMatrix(volatileContext, commands);
  _updatePrevTransformMatrix(volatileContext, commands);
}

void MeshDrawCommand::_updateTransformMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  if(!_drawInfo.transformMatrix->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.transformMatrix();
  }

  _drawInfo.transformMatrix->setValue(volatileContext,
                                      UniformVariable::ValueRef{
                                                      .data = positionMatrices,
                                                      .dataSize = dataSize});
}

void MeshDrawCommand::_updatePrevTransformMatrix(
                                      TechniqueVolatileContext& volatileContext,
                                      std::span<const CommandPtr> commands)
{
  if(!_drawInfo.prevTransformMatrix->isActive()) return;

  size_t arraySize = commands.size();
  size_t dataSize = arraySize * sizeof(glm::mat4);
  glm::mat4* positionMatrices = (glm::mat4*)alloca(dataSize);
  for(size_t i = 0; i < arraySize; i++)
  {
    const MeshDrawCommand& meshCommand =
                              static_cast<const MeshDrawCommand&>(*commands[i]);
    positionMatrices[i] = meshCommand._drawable.prevTransformMatrix();
  }

  _drawInfo.prevTransformMatrix->setValue(volatileContext,
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