#pragma once

#include <cstdint>

#include <hld/drawCommand/DrawCommand.h>
#include <hld/meshDrawable/MeshDrawInfo.h>

namespace mt
{
  class MeshDrawable;
  class Technique;
  class TechniquePass;
  class TechniqueVolatileContext;
  class UniformVariable;

  class MeshDrawCommand : public DrawCommand
  {
  public:
    inline MeshDrawCommand( const MeshDrawable& drawable,
                            const MeshDrawInfo& drawInfo,
                            uint32_t vertexCount,
                            float distance);
    MeshDrawCommand(const MeshDrawCommand&) = delete;
    MeshDrawCommand& operator = (const MeshDrawCommand&) = delete;
    virtual ~MeshDrawCommand() noexcept = default;

    virtual void draw(CommandProducerGraphic& producer,
                      std::span<const CommandPtr> commands) override;

  protected:
    virtual void updateInstanceData(TechniqueVolatileContext& volatileContext,
                                    std::span<const CommandPtr> commands);

  private:
    void _processChunk( CommandProducerGraphic& producer,
                        std::span<const CommandPtr> commands);
    void _updatePositionMatrix( TechniqueVolatileContext& volatileContext,
                                std::span<const CommandPtr> commands);
    void _updatePrevPositionMatrix( TechniqueVolatileContext& volatileContext,
                                    std::span<const CommandPtr> commands);
    void _updateBivecMatrix(TechniqueVolatileContext& volatileContext,
                            std::span<const CommandPtr> commands);

  private:
    const MeshDrawable& _drawable;
    const MeshDrawInfo& _drawInfo;
    uint32_t _vertexCount;
  };

  inline MeshDrawCommand::MeshDrawCommand(const MeshDrawable& drawable,
                                          const MeshDrawInfo& drawInfo,
                                          uint32_t vertexCount,
                                          float distance) :
    DrawCommand(drawInfo.commandGroup, drawInfo.layer, distance),
    _drawable(drawable),
    _drawInfo(drawInfo),
    _vertexCount(vertexCount)
  {
  }
}