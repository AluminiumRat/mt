#pragma once

#include <hld/drawCommand/DrawCommand.h>
#include <util/Ref.h>

namespace mt
{
  class MeshDrawable;
  class Technique;

  class MeshDrawCommand : public DrawCommand
  {
  public:
    inline MeshDrawCommand( const MeshDrawable& drawable,
                            const TechniquePass& pass,
                            uint32_t vertexCount,
                            uint32_t maxInstances,
                            Group groupIndex,
                            int32_t layer,
                            float distance);
    MeshDrawCommand(const MeshDrawCommand&) = delete;
    MeshDrawCommand& operator = (const MeshDrawCommand&) = delete;
    virtual ~MeshDrawCommand() noexcept = default;

    virtual void draw(CommandProducerGraphic& producer,
                      std::span<const CommandPtr> commands) override;

  private:
    void _processChunk( CommandProducerGraphic& producer,
                        const Technique& technique,
                        std::span<const CommandPtr> commands);
    void _updatePositionMatrix( TechniqueVolatileContext& volatileContext,
                                std::span<const CommandPtr> commands);
    void _updatePrevPositionMatrix( TechniqueVolatileContext& volatileContext,
                                    std::span<const CommandPtr> commands);
    void _updateBivecMatrix(TechniqueVolatileContext& volatileContext,
                            std::span<const CommandPtr> commands);

  private:
    const MeshDrawable& _drawable;
    const TechniquePass& _pass;
    uint32_t _vertexCount;
    size_t _maxInstances;
  };

  inline MeshDrawCommand::MeshDrawCommand(const MeshDrawable& drawable,
                                          const TechniquePass& pass,
                                          uint32_t vertexCount,
                                          uint32_t maxInstances,
                                          Group groupIndex,
                                          int32_t layer,
                                          float distance) :
    DrawCommand(groupIndex, layer, distance),
    _drawable(drawable),
    _pass(pass),
    _vertexCount(vertexCount),
    _maxInstances(maxInstances)
  {
  }
}