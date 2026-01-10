#pragma once

#include <filesystem>
#include <unordered_set>

#include <technique/TechniqueConfigurator.h>
#include <util/Ref.h>
#include <vkr/Sampler.h>

namespace YAML
{
  class Node;
}

namespace mt
{
  class Device;

  using TechniqueFiles = std::unordered_set<std::filesystem::path>;

  //  Загрузить настройки техники из файла
  //  Загружается полностью, либо вызывается TechniqueConfigurator::clear и
  //    выбрасывается исключение
  void loadConfigurator(TechniqueConfigurator& target,
                        const std::filesystem::path& file);

  //  То же самое, что и предыдущая функция, но добавляет в usedFiles имена
  //    файлов, которые были использованы (включая параметр file)
  //  usedFiles будет заполняться и в случае, если во время загрузки произойдет
  //    какая-либо ошибка и будет вызвано прерывание, в этом случае в usedFiles
  //    будут добавлены те файлы, которые были найдены до ошибки
  //  Можно передать usedFiles = nullptr, тогда функция работает в точности так
  //    же, как и предыдущая.
  //  ВНИМАНИЕ! usedFiles не очищается внутри метода, а дополняется
  void loadConfigurator(TechniqueConfigurator& target,
                        const std::filesystem::path& file,
                        std::unordered_set<std::filesystem::path>* usedFiles);

  //  Загрузить настройки сэмплера
  //  Без ссылок на внешние файлы и наследования свойств
  SamplerDescription loadSamplerDescription(const YAML::Node& samplerNode);
}