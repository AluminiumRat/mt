#pragma once

#include <filesystem>

#include <technique/TechniqueConfigurator.h>
#include <util/Ref.h>
#include <vkr/Sampler.h>

namespace mt
{
  class Device;

  //  Загрузить настройки техники из файла
  void loadConfigurator(TechniqueConfigurator& target,
                        const std::filesystem::path& file);
}