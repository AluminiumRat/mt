// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>

#include <util/Assert.h>
#include <vkr/pipeline/ShaderLoader.h>

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

std::vector<uint32_t> DefaultShaderLoader::loadShader(const char* filename)
{
  // Для начала ищем в текущей папке
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
  {
    // В подкаталоге shaders
    file.open(std::string("shaders/") + filename, std::ios::ate | std::ios::binary);
  }
  if (!file.is_open())
  {
    // Парсим переменную окружения и ищем по путям в ней
    const char* patchesEnv = std::getenv("MT_SHADERS");
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
    throw std::runtime_error(std::string("Unable to open file :") + filename);
  }

  // Определяем размер данных
  size_t dataSize = (size_t)file.tellg();
  if(dataSize == 0)
  {
    throw std::runtime_error(std::string("File ") + filename + " is empty");
  }
  size_t vectorSize = dataSize / sizeof(uint32_t);
  if(dataSize % sizeof(uint32_t) != 0) vectorSize++;

  // Считываем данные
  std::vector<uint32_t> dataVector;
  dataVector.resize(vectorSize);
  file.seekg(0);
  file.read((char*)(dataVector.data()), dataSize);

  return dataVector;
}
