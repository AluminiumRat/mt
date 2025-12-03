// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>

#include <util/Assert.h>
#include <ddsSupport/ContentLoader.h>

using namespace mt;

static std::unique_ptr<ContentLoader> contentLoader(new DefaultContentLoader);

void ContentLoader::setContentLoader(std::unique_ptr<ContentLoader> newLoader)
{
  MT_ASSERT(newLoader != nullptr);
  contentLoader = std::move(newLoader);
}

ContentLoader& ContentLoader::getContentLoader() noexcept
{
  return *contentLoader;
}

static std::ifstream openFile(const char* filename)
{
  // Для начала ищем в текущей папке
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
  {
    // В подкаталоге shaders
    file.open(std::string("content/") + filename,
              std::ios::ate | std::ios::binary);
  }
  if (!file.is_open())
  {
    // Парсим переменную окружения и ищем по путям в ней
    const char* patchesEnv = std::getenv("MT_CONTENT_DIRS");
    if(patchesEnv != nullptr)
    {
      std::istringstream envStream(patchesEnv);
      std::string path;
      while(std::getline(envStream, path, ';'))
      {
        file.open(path + "/" + filename, std::ios::ate | std::ios::binary);
        if (file.is_open()) break;
      }
    }
  }

  if (!file.is_open())
  {
    throw std::runtime_error(std::string("Unable to open file: ") + filename);
  }

  return file;
}

std::vector<char> DefaultContentLoader::loadData(const char* filename)
{
  std::ifstream file = openFile(filename);

  // Определяем размер данных
  size_t dataSize = (size_t)file.tellg();
  if(dataSize == 0) return {};

  // Считываем данные
  std::vector<char> result;
  result.resize(dataSize);
  file.seekg(0);
  file.read(result.data(), dataSize);

  return result;
}