// Отключаем Warning на std::getenv
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <util/Abort.h>
#include <util/Log.h>
#include <vkr/pipeline/ShaderModule.h>
#include <vkr/Device.h>

using namespace mt;

// Дефолтный лоадер для шейдеров
// Ищет файл в файловой системе в текущей папке, в текущей папке в каталоге
// shaders и в путях, указанных в переменной окружения MT_SHADERS(пути разделены
// точкой с запятой)
class DefaultShaderLoader
{
public:
  virtual std::vector<uint32_t> loadShader(const char* filename)
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
};

static DefaultShaderLoader defaultLoader;
static DefaultShaderLoader* shaderLoader = &defaultLoader;

ShaderModule::ShaderModule( Device& device,
                            std::span<const uint32_t> spvData,
                            const char* debugName):
  _device(device),
  _handle(VK_NULL_HANDLE),
  _debugName(debugName)
{
  try
  {
    _createHandle(spvData);
  }
  catch(...)
  {
    _cleanup();
    throw;
  }
}

ShaderModule::ShaderModule(Device& device, const char* spvFilename) :
  _device(device),
  _handle(VK_NULL_HANDLE),
  _debugName(spvFilename)
{
  try
  {
    std::vector<uint32_t> spvData = shaderLoader->loadShader(spvFilename);
    _createHandle(spvData);
  }
  catch (...)
  {
    _cleanup();
    throw;
  }
}

void ShaderModule::_createHandle(std::span<const uint32_t> spvData)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = spvData.size() * 4;
  createInfo.pCode = spvData.data();
  if (vkCreateShaderModule( _device.handle(),
                            &createInfo,
                            nullptr,
                            &_handle) != VK_SUCCESS)
  {
    throw std::runtime_error(std::string("ShaderModule: Failed to create shader module: ") + _debugName);
  }
}

ShaderModule::~ShaderModule() noexcept
{
  _cleanup();
}

void ShaderModule::_cleanup() noexcept
{
  if(_handle != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(_device.handle(), _handle, nullptr);
    _handle = VK_NULL_HANDLE;
  }
}
