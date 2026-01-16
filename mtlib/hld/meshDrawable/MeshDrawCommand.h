#pragma once

#include <hld/drawCommand/DrawCommand.h>
#include <technique/Technique.h>
#include <util/Ref.h>

namespace mt
{
  class MeshDrawCommand : public DrawCommand
  {
  public:
    MeshDrawCommand(const Technique& technique,
                    const TechniquePass& pass,
                    uint32_t vertexCount,
                    uint32_t maxInstances,
                    uint32_t groupIndex,
                    int32_t layer,
                    float distance);
    MeshDrawCommand(const MeshDrawCommand&) = delete;
    MeshDrawCommand& operator = (const MeshDrawCommand&) = delete;
    virtual ~MeshDrawCommand() noexcept = default;

    virtual void draw(CommandProducerGraphic& producer,
                      std::span<const CommandPtr> commands) override;

  private:
    void _processChunk( CommandProducerGraphic& producer,
                        std::span<const CommandPtr> commands);

  private:
    ConstRef<Technique> _technique;
    const TechniquePass& _pass;
    uint32_t _vertexCount;
    size_t _maxInstances;
  };
}