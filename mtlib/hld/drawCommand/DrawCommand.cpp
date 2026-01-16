#include <hld/drawCommand/DrawCommand.h>

using namespace mt;

DrawCommand::DrawCommand( uint32_t groupIndex,
                          int32_t layer,
                          float distance) noexcept :
  _groupIndex(groupIndex),
  _layer(layer),
  _distance(distance)
{
}
