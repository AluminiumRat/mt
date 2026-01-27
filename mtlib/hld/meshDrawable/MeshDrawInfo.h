#pragma once

#include <cstdint>

#include <hld/drawCommand/DrawCommand.h>

namespace mt
{
  class Technique;
  class TechniquePass;
  class UniformVariable;

  //  Информация об отдельном проходе отрисовки меша
  //  Хранится внутри MeshAsset, используется MeshDrawCommand
  //  Пользователь не должен взаимодействовать с этими данными напрямую
  struct MeshDrawInfo
  {
    int32_t layer;
    DrawCommand::Group commandGroup;

    Technique* technique;

    TechniquePass* pass;
    const UniformVariable* positionMatrix;
    const UniformVariable* prevPositionMatrix;
    const UniformVariable* bivecMatrix;
  };
}