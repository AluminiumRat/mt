#pragma once

#include <filesystem>
#include <span>
#include <unordered_set>
#include <vector>

#include <vulkan/vulkan.h>

namespace mt
{
  // Интеграция shaderC
  class ShaderCompilator
  {
  public:
    struct Define
    {
      const char* name;
      const char* value;
    };

    //  Скомпилировать шейдер из файла file
    //  Метод либо возвращает валидный бинарный код, либо кидает исключение
    //  usedFiles - контейнер, в который будут добавляться все использованные
    //    при компиляции файлы, включая file. Можно передать nullptr.
    //  Если скомпилировать шейдер не удалось, и метод выбрасывает исключение,
    //    то в контейнер usedFiles всё равно будут добавлены те файлы, которые
    //    удалось распознать.
    //  ВНИМАНИЕ! Метод не чистит usedFiles перед работой, только добавляет
    //    новые файлы.
    static std::vector<char> compile(
                        const std::filesystem::path& file,
                        VkShaderStageFlagBits shaderStage,
                        std::span<const Define> defines,
                        std::unordered_set<std::filesystem::path>* usedFiles);
  };
}