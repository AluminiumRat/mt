#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <technique/ShaderCompilator.h>
#include <technique/TechniqueGonfiguration.h>
#include <vkr/pipeline/ShaderModule.h>

namespace mt
{
  class Device;

  //  Промежуточные данные, необходимые для построения конфигурации
  //  Один собранный шейдерный модуль
  //  Внутренняя кухня TechniqueConfigurator. Пользователь не должен
  //    взаимодействовать с этими данными.
  struct ShaderModuleInfo
  {
    std::unique_ptr<ShaderModule> shaderModule;
    VkShaderStageFlagBits stage;
  };
  //  Набор шейдерных модулей для одного варианта пайплайна
  using ShaderSet = std::vector<ShaderModuleInfo>;
  //  Набор различных вариантов шейдеров для одного прохода
  using PassShaderVariants = std::vector<ShaderSet>;

  //  Промежуточные данные, необходимые для построения конфигурации
  //    техники.
  //  Внутренняя кухня TechniqueConfigurator. Пользователь не должен
  //    взаимодействовать с этими данными.
  struct ConfigurationBuildContext
  {
    Device* device = nullptr;
    std::string configuratorName;

    //  Конфигурация, которую мы создаем
    Ref<TechniqueGonfiguration> configuration;

    //  Все варианты шейдеров для всех проходов
    std::vector<PassShaderVariants> shaders;

    uint32_t currentPassIndex = 0;
    PassConfiguration* currentPass = nullptr;

    //  Текущий обрабатываемый вариант пайплайна
    uint32_t currentVariantIndex = 0;
    std::vector<ShaderCompilator::Define> currentDefines;
  };
}