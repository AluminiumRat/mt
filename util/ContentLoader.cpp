// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <util/Assert.h>
#include <util/ContentLoader.h>

namespace fs = std::filesystem;

using namespace mt;

static std::unique_ptr<ContentLoader> contentLoader(new DefaultContentLoader);

void ContentLoader::setLoader(std::unique_ptr<ContentLoader> newLoader)
{
  MT_ASSERT(newLoader != nullptr);
  contentLoader = std::move(newLoader);
}

ContentLoader& ContentLoader::getLoader() noexcept
{
  return *contentLoader;
}

static std::ifstream openFile(const fs::path& file,
                              const std::vector<fs::path>& searchPatches)
{
  std::ifstream fileStream;
  if(file.is_absolute())
  {
    //  Путь абсолютный, пытаемся открыть как есть
    fileStream.open(file, std::ios::ate | std::ios::binary);
  }
  else
  {
    //  Перебираем пути, где может лежать файл, и пытаемся открыть
    for(const fs::path& path : searchPatches)
    {
      fileStream.open(path / file, std::ios::ate | std::ios::binary);
      if(fileStream.is_open()) break;
    }
  }
  if (!fileStream.is_open())
  {
    throw std::runtime_error(std::string("Unable to open file: ") + (const char*)file.u8string().c_str());
  }

  return fileStream;
}

DefaultContentLoader::DefaultContentLoader()
{
  _searchPatches.push_back(fs::current_path());
  _searchPatches.push_back(fs::current_path() / "content");

  // Парсим переменную окружения и ищем по путям в ней
  const char* patchesEnv = std::getenv("MT_CONTENT_DIRS");
  if(patchesEnv != nullptr)
  {
    std::istringstream envStream(patchesEnv);
    std::string path;
    while(std::getline(envStream, path, ';'))
    {
      if(!path.empty()) _searchPatches.push_back(path);
    }
  }
}

std::vector<char> DefaultContentLoader::loadData(const fs::path& file)
{
  std::ifstream fileStream = openFile(file, _searchPatches);

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
  std::ifstream fileStream = openFile(file, _searchPatches);

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

bool DefaultContentLoader::exists(const fs::path& file)
{
  if(file.is_absolute()) return fs::exists(file);

  for (const fs::path& path : _searchPatches)
  {
    if(fs::exists(path / file)) return true;
  }

  return false;
}

fs::file_time_type DefaultContentLoader::lastWriteTime(const fs::path& file)
{
  if(file.is_absolute()) return fs::last_write_time(file);

  for (const fs::path& path : _searchPatches)
  {
    fs::path fullFilePath = path / file;
    if (fs::exists(fullFilePath)) return fs::last_write_time(fullFilePath);
  }

  throw std::runtime_error(std::string("DefaultContentLoader::lastWriteTime: file " + std::string((const char*)file.u8string().c_str())) + " doesn't exist");
}
