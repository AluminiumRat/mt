#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace mt
{
  //  Интерфейс загрузчика данных для контента. От него наследуются конкретные
  //    загрузчики, настроенные под конкретное приложение.
  //  Используйте ContentLoader::setLoader и ContentLoader::getLoader
  //    для установки/получения текущего используемого лоадера
  class ContentLoader
  {
  public:
    virtual ~ContentLoader() noexcept = default;
    //  Эти методы должны быть потокобезопасными
    //  Загрузить файл просто как байтовый набор
    virtual std::vector<char> loadData(const std::filesystem::path& file) = 0;
    //  Загрузить файл как текстовую строку
    virtual std::string loadText(const std::filesystem::path& file) = 0;
    //  Существует ли вообще файл
    virtual bool exists(const std::filesystem::path& file) = 0;
    //  Время последней модификации
    virtual std::filesystem::file_time_type lastWriteTime(
                                        const std::filesystem::path& file) = 0;

    //  Установить загрузчик данных
    //  По умолчанию установлен DefaultContentLoader
    //  ВНИМАНИЕ!!! Метод не потокобезопасный. Предполагается, что лоадер будет
    //    установлен один раз при инициализации приложения
    static void setLoader(std::unique_ptr<ContentLoader> newLoader);
    static ContentLoader& getLoader() noexcept;
  };

  //  Дефолтный лоадер
  //  Если путь абсолютный, то ищет файл в файловой системе как есть, если путь
  //    относительный, то ищет в каталоге приложения (вернее в каталоге,
  //    который был текущим на момент создания DefaultContentLoader), в
  //    подкаталогах shaders, content  и content/shaders а также в путях,
  //    указанных в переменной окружения MT_CONTENT_DIRS(пути разделены точкой
  //    с запятой)
  class DefaultContentLoader : public ContentLoader
  {
  public:
    DefaultContentLoader();
    DefaultContentLoader(const DefaultContentLoader&) = default;
    DefaultContentLoader& operator = (const DefaultContentLoader&) = default;
    virtual ~DefaultContentLoader() noexcept = default;

    virtual std::vector<char> loadData(
                                    const std::filesystem::path& file) override;
    virtual std::string loadText(const std::filesystem::path& file) override;
    virtual bool exists(const std::filesystem::path& file) override;
    virtual std::filesystem::file_time_type lastWriteTime(
                                    const std::filesystem::path& file) override;
  private:
    std::vector<std::filesystem::path> _searchPatches;
  };
}