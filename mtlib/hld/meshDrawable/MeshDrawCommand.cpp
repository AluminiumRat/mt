#include <hld/meshDrawable/MeshDrawCommand.h>
#include <vkr/queue/CommandProducerGraphic.h>

using namespace mt;

void MeshDrawCommand::draw( CommandProducerGraphic& producer,
                            std::span<const CommandPtr> commands)
{
  if(!_technique->isReady()) return;

  Technique::Bind bind(*_technique, _pass, producer);
  if(!bind.isValid()) return;

  size_t commandProcessed = 0;
  while(commandProcessed != commands.size())
  {
    size_t chunkSize = commands.size() - commandProcessed;
    chunkSize = std::min(chunkSize, _maxInstances);
    _processChunk(producer, commands.subspan(commandProcessed, chunkSize));
    commandProcessed += chunkSize;
  }
}

void MeshDrawCommand::_processChunk(CommandProducerGraphic& producer,
                                    std::span<const CommandPtr> commands)
{
  producer.draw(_vertexCount, (uint32_t)commands.size());
}
