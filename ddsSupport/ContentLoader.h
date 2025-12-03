#pragma once

#include <cstdlib>
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
    //  Этот метод должен быть потокобезопасным
    virtual std::vector<char> loadData(const char* filename) = 0;

    //  Установить загрузчик данных
    //  По умолчанию установлен DefaultContentLoader
    //  ВНИМАНИЕ!!! Метод не потокобезопасный. Предполагается, что лоадер будет
    //    установлен один раз при инициализации приложения
    static void setContentLoader(std::unique_ptr<ContentLoader> newLoader);
    static ContentLoader& getContentLoader() noexcept;
  };

  //  Дефолтный лоадер
  //  Ищет файл в файловой системе в текущей папке, в текущей папке в каталоге
  //    content и в путях, указанных в переменной окружения MT_CONTENT_DIRS(пути
  //    разделены точкой с запятой)
  class DefaultContentLoader : public ContentLoader
  {
  public:
    virtual std::vector<char> loadData(const char* filename) override;
  };
}