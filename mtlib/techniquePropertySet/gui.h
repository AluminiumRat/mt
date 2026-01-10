#pragma once

namespace mt
{
  class TechniquePropertySet;
  class TechniqueProperty;

  //  Добавить в текущий контекст ImGui грид с настройками техники
  void techniquePropertyGrid(const char* id, TechniquePropertySet& propertySet);
};