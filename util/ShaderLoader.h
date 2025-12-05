#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace mt
{
  //  Интерфейс загрузчика данных для шейдера. От него наследуются конкретные
  //   загрузчики, настроенные под конкретное приложение.
  //  Используйте ShaderLoader::setLoader и ShaderLoader::getLoader
  //    для установки/получения текущего используемого лоадера
  class ShaderLoader
  {
  public:
    virtual ~ShaderLoader() noexcept = default;
    //  Этот метод должен быть потокобезопасным
    virtual std::vector<uint32_t> loadSPIRV(
                                        const std::filesystem::path& file) = 0;
    //  Этот метод должен быть потокобезопасным
    virtual std::string loadText(const std::filesystem::path& file) = 0;

    //  Установить загрузчик данных для шейдеров
    //  По умолчанию установлен DefaultShaderLoader
    //  ВНИМАНИЕ!!! Метод не потокобезопасный. Предполагается, что лоадер будет
    //    установлен один раз при инициализации приложения
    static void setShaderLoader(std::unique_ptr<ShaderLoader> newLoader);
    static ShaderLoader& getShaderLoader() noexcept;
  };

  //  Дефолтный лоадер для шейдеров
  //  Ищет файл в файловой системе в текущей папке, в текущей папке в каталоге
  //    shaders и в путях, указанных в переменной окружения MT_SHADERS(пути
  //    разделены точкой с запятой)
  class DefaultShaderLoader : public ShaderLoader
  {
  public:
    virtual std::vector<uint32_t> loadSPIRV(
                                    const std::filesystem::path& file) override;
    virtual std::string loadText(const std::filesystem::path& file) override;
  };
}