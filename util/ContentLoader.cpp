// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>

#include <util/Assert.h>
#include <util/ContentLoader.h>

namespace fs = std::filesystem;

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

static std::ifstream openFile(const fs::path& file)
{
  // Для начала ищем в текущей папке
  std::ifstream fileStream(file, std::ios::ate | std::ios::binary);
  if (!fileStream.is_open())
  {
    // В подкаталоге shaders
    fileStream.open(fs::path("content") / file,
                    std::ios::ate | std::ios::binary);
  }
  if (!fileStream.is_open())
  {
    // Парсим переменную окружения и ищем по путям в ней
    const char* patchesEnv = std::getenv("MT_CONTENT_DIRS");
    if(patchesEnv != nullptr)
    {
      std::istringstream envStream(patchesEnv);
      std::string path;
      while(std::getline(envStream, path, ';'))
      {
        fileStream.open(fs::path(path) / file,
                        std::ios::ate | std::ios::binary);
        if (fileStream.is_open()) break;
      }
    }
  }

  if (!fileStream.is_open())
  {
    throw std::runtime_error(std::string("Unable to open file: ") + (const char*)file.u8string().c_str());
  }

  return fileStream;
}

std::vector<char> DefaultContentLoader::loadData(const fs::path& file)
{
  std::ifstream fileStream = openFile(file);

  // Определяем размер данных
  size_t dataSize = (size_t)fileStream.tellg();
  if(dataSize == 0) return {};

  // Считываем данные
  std::vector<char> result;
  result.resize(dataSize);
  fileStream.seekg(0);
  fileStream.read(result.data(), dataSize);

  return result;
}

std::string DefaultContentLoader::loadText(const fs::path& file)
{
  std::ifstream fileStream = openFile(file);

  // Определяем размер данных
  size_t dataSize = (size_t)fileStream.tellg();
  if (dataSize == 0) return std::string();

  // Считываем данные
  std::string result;
  result.resize(dataSize);
  fileStream.seekg(0);
  fileStream.read((char*)(result.data()), dataSize);

  return result;
}