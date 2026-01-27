#pragma once

#include <cstdint>

#include <hld/drawCommand/DrawCommand.h>

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
                            const Technique& technique,
                            const TechniquePass& pass,
                            const UniformVariable& positionMatrix,
                            const UniformVariable& prevPositionMatrix,
                            const UniformVariable& bivecMatrix,
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
    const Technique& _technique;
    const TechniquePass& _pass;
    const UniformVariable& _positionMatrix;
    const UniformVariable& _prevPositionMatrix;
    const UniformVariable& _bivecMatrix;
    uint32_t _vertexCount;
    size_t _maxInstances;
  };

  inline MeshDrawCommand::MeshDrawCommand(
                                      const MeshDrawable& drawable,
                                      const Technique& technique,
                                      const TechniquePass& pass,
                                      const UniformVariable& positionMatrix,
                                      const UniformVariable& prevPositionMatrix,
                                      const UniformVariable& bivecMatrix,
                                      uint32_t vertexCount,
                                      uint32_t maxInstances,
                                      Group groupIndex,
                                      int32_t layer,
                                      float distance) :
    DrawCommand(groupIndex, layer, distance),
    _drawable(drawable),
    _technique(technique),
    _pass(pass),
    _positionMatrix(positionMatrix),
    _prevPositionMatrix(prevPositionMatrix),
    _bivecMatrix(bivecMatrix),
    _vertexCount(vertexCount),
    _maxInstances(maxInstances)
  {
  }
}