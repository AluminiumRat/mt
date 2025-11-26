#pragma once

#include <yaml-cpp/yaml.h>

#include <technique/TechniqueConfigurator.h>
#include <util/Ref.h>
#include <vkr/Sampler.h>

namespace mt
{
  class Device;

  //  Загрузить настройки техники из файла
  void loadConfigurator(TechniqueConfigurator& target, const char* filename);

  //  Загрузить настройки техники из отдельной YAML ноды
  void loadConfigurator(TechniqueConfigurator& target, YAML::Node node);

  //  Загрузить список селекшенов конфигурации
  TechniqueConfigurator::Selections loadSelections(YAML::Node selectionsNode);

  //  Загрузить один отдельный проход
  void loadPass(YAML::Node passNode, PassConfigurator& target);

  //  Загрузить сэмплер
  ConstRef<Sampler> loadSampler(YAML::Node samplerNode, Device& device);
}