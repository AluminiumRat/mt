// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>

#include <util/Assert.h>
#include <util/ShaderLoader.h>

namespace fs = std::filesystem;

using namespace mt;

static std::unique_ptr<ShaderLoader> shaderLoader(new DefaultShaderLoader);

void ShaderLoader::setShaderLoader(std::unique_ptr<ShaderLoader> newLoader)
{
  MT_ASSERT(newLoader != nullptr);
  shaderLoader = std::move(newLoader);
}

ShaderLoader& ShaderLoader::getShaderLoader() noexcept
{
  return *shaderLoader;
}

static std::ifstream openFile(const fs::path& file)
{
  // Для начала ищем в текущей папке
  std::ifstream fileStream(file, std::ios::ate | std::ios::binary);
  if (!fileStream.is_open())
  {
    // В подкаталоге shaders
    fileStream.open(fs::path("shaders") / file,
                    std::ios::ate | std::ios::binary);
  }
  if (!fileStream.is_open())
  {
    // Парсим переменную окружения и ищем по путям в ней
    const char* patchesEnv = std::getenv("MT_SHADERS");
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

std::vector<uint32_t> DefaultShaderLoader::loadSPIRV(const fs::path& file)
{
  std::ifstream fileStream = openFile(file);

  // Определяем размер данных
  size_t dataSize = (size_t)fileStream.tellg();
  if(dataSize == 0) throw std::runtime_error(std::string("File ") + (const char*)file.u8string().c_str() + " is empty");

  size_t vectorSize = dataSize / sizeof(uint32_t);
  if(dataSize % sizeof(uint32_t) != 0) vectorSize++;

  // Считываем данные
  std::vector<uint32_t> dataVector;
  dataVector.resize(vectorSize);
  fileStream.seekg(0);
  fileStream.read((char*)(dataVector.data()), dataSize);

  return dataVector;
}

std::string DefaultShaderLoader::loadText(const fs::path& file)
{
  std::ifstream fileStream = openFile(file);

  // Определяем размер данных
  size_t dataSize = (size_t)fileStream.tellg();
  if (dataSize == 0) throw std::runtime_error(std::string("File ") + (const char*)file.u8string().c_str() + " is empty");

  // Считываем данные
  std::string result;
  result.resize(dataSize);
  fileStream.seekg(0);
  fileStream.read((char*)(result.data()), dataSize);

  return result;
}