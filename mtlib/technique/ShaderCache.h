#pragma once

#include <fstream>
#include <map>

#include <technique/ShaderCompilator.h>

namespace mt
{
  //  Штука, которая сохраняет во временной папке собранный SPIR-V код и
  //  считывает его оттуда по запросу.
  class ShaderCache
  {
  public:
    //  Набор пар "имя файла"/"время последнего обновления", которые
    //    участвовали в создании шейдерного кода
    //  Необходим для валидации кэша
    using Resources = std::map< std::filesystem::path,
                                std::filesystem::file_time_type>;

  public:
    ShaderCache();
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator = (const ShaderCache&) = delete;
    ~ShaderCache() noexcept = default;

    //  Закэшировать собранный код
    //  shaderFile - имя исходного GLSL файла, для которого сгенерирован код
    //  shaderStage и defines вместе с file используются как ключ, отличающий
    //    кэши друг от друга
    //  usedFiles используется для валидации кэша и для восстановления
    //    списка файлов, от которых зависит SPIR-V код. Этот список должен
    //    включать в себя file
    void save(std::span<const char> spirCode,
              const std::filesystem::path& shaderFile,
              VkShaderStageFlagBits shaderStage,
              std::span<const ShaderCompilator::Define> defines,
              const Resources& usedFiles) noexcept;

    //  Загрузить ранее сохраненный SPIRV код (если он есть и валиден)
    //  shaderFile - имя исходного GLSL файла, для которого необходимо получить
    //    SPIR-V код
    //  shaderStage и defines вместе с file используются как ключ, отличающий
    //    кэши друг от друга
    //  В usedFiles будут добавлены файлы, от которых зависит SPIR-V код,
    //    включая file. Метод не чистит usedFiles перед работой, только
    //    добавляет новые файлы. Если кэш загрузить не удалось, либо он
    //    невалидный, то никакие новые файлы в usedFiles добавлены не будут
    //  В случае, если не удалось загрузить кэш или он невалиден, метод
    //    вернет пустой контейнер
    std::vector<char> load(
          const std::filesystem::path& shaderFile,
          VkShaderStageFlagBits shaderStage,
          std::span<const ShaderCompilator::Define> defines,
          std::unordered_set<std::filesystem::path>* usedFiles) const noexcept;
  private:
    std::filesystem::path _getCacheFilename(
                      const std::filesystem::path& shaderFile,
                      VkShaderStageFlagBits shaderStage,
                      std::span<const ShaderCompilator::Define> defines) const;
    bool _readAndCheckFileHeader(std::ifstream& fileStream) const;
    bool _readAndCheckCompilatorVersion(std::ifstream& fileStream) const;
    bool _readAndCheckFilename( std::ifstream& fileStream,
                                const std::filesystem::path& shaderFile) const;
    bool _readAndCheckStage(std::ifstream& fileStream,
                            VkShaderStageFlagBits shaderStage,
                            const std::filesystem::path& shaderFile) const;
    bool _readAndCheckDefines(std::ifstream& fileStream,
                              std::span<const ShaderCompilator::Define> defines,
                              const std::filesystem::path& shaderFile) const;
    bool _readAndCheckUsedFiles(
                      std::ifstream& fileStream,
                      std::unordered_set<std::filesystem::path>& targetSet) const;
    std::vector<char> _readSpirV(std::ifstream& fileStream) const;

  private:
    std::filesystem::path _cacheFolder;
  };
}