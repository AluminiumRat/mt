#include <stdexcept>
#include <string>

#include <technique/ShaderCache.h>
#include <util/Assert.h>
#include <util/ContentLoader.h>
#include <util/fileSystemHelpers.h>
#include <util/Log.h>

namespace fs = std::filesystem;

using namespace mt;

//  Магические 4 байта в начале файла, чтобы отличать свои файлы от мусора
const char cacheHeader[4] = {'m', 't', 's', 'c'};

ShaderCache::ShaderCache()
{
  _cacheFolder = fs::temp_directory_path() / "mtShaderCache";
}

void ShaderCache::save(
                  std::span<const char> spirCode,
                  const std::filesystem::path& shaderFile,
                  VkShaderStageFlagBits shaderStage,
                  std::span<const ShaderCompilator::Define> defines,
                  const Resources& usedFiles) noexcept
{
  MT_ASSERT(!spirCode.empty());
  MT_ASSERT(!shaderFile.empty());

  try
  {
    //  Открываем файл для записи
    fs::create_directories(_cacheFolder);
    fs::path filename = _getCacheFilename(shaderFile, shaderStage, defines);
    std::ofstream fileStream;
    fileStream.open(filename, std::ios::binary);
    if(!fileStream.is_open()) throw std::runtime_error(std::string("Unable to open ") + pathToUtf8(filename));

    //  Записываем невалидный хидер на случай ошибок во время записи. В конце
    //  заменим его на правильный
    fileStream << 'n' << 'n' << 'n' << 'n';

    //  Заполняем файл
    fileStream << ShaderCompilator::version << std::endl;
    fileStream << shaderFile << std::endl;
    fileStream << (int32_t)shaderStage << std::endl;
    fileStream << (uint32_t)defines.size() << std::endl;
    for(const ShaderCompilator::Define& defineInfo : defines)
    {
      fileStream << defineInfo.name;
      fileStream << ' ';
      fileStream << defineInfo.value << std::endl;
    }
    //  Файлы, участвовавшие в построении шейдера
    fileStream << (uint32_t)usedFiles.size() << std::endl;
    for(Resources::const_iterator iResource = usedFiles.begin();
        iResource != usedFiles.end();
        iResource++)
    {
      //  Имя файла
      fileStream << iResource->first;
      fileStream << " ";
      //  Время последней модификации в unix time
      auto ftime = std::chrono::clock_cast<std::chrono::system_clock>(
                                                            iResource->second);
      fileStream << std::chrono::system_clock::to_time_t(ftime);
      fileStream << std::endl;
    }
    fileStream << spirCode.size() << ' ';
    fileStream.write(spirCode.data(), spirCode.size());

    //  Всё прошло успешно, запишем валдный хидер
    fileStream.seekp(0);
    fileStream.write(cacheHeader, sizeof(cacheHeader));
  }
  catch(std::exception& error)
  {
    Log::error() << "ShaderCache::save: " << error.what();
  }
}

fs::path ShaderCache::_getCacheFilename(
                        const fs::path& shaderFile,
                        VkShaderStageFlagBits shaderStage,
                        std::span<const ShaderCompilator::Define> defines) const
{
  std::string description = pathToUtf8(shaderFile) +
                                              std::to_string((int)shaderStage);
  for(const ShaderCompilator::Define& defineInfo : defines)
  {
    description += defineInfo.name;
    description += defineInfo.value;
  }
  size_t keyHash = std::hash<std::string>()(description);
  keyHash = keyHash ^ ContentLoader::getLoader().loaderHash();

  std::string filename = std::to_string(keyHash) + ".shc";
  return _cacheFolder / filename;
}

std::vector<char> ShaderCache::load(
            const std::filesystem::path& shaderFile,
            VkShaderStageFlagBits shaderStage,
            std::span<const ShaderCompilator::Define> defines,
            std::unordered_set<std::filesystem::path>* usedFiles) const noexcept
{
  try
  {
    fs::path filename = _getCacheFilename(shaderFile, shaderStage, defines);
    std::ifstream fileStream(filename, std::ios::binary);
    if(!fileStream.is_open()) return {};

    if(!_readAndCheckFileHeader(fileStream)) return {};
    if(!_readAndCheckCompilatorVersion(fileStream)) return {};
    if(!_readAndCheckFilename(fileStream, shaderFile)) return {};
    if(!_readAndCheckStage(fileStream, shaderStage, shaderFile)) return {};
    if(!_readAndCheckDefines(fileStream, defines, shaderFile)) return {};

    //  Временное хранилище. Перекидывать в usedFiles будем только когда
    //  полностью обработаем файл
    std::unordered_set<fs::path> tmpUsedFiles;
    if(!_readAndCheckUsedFiles(fileStream, tmpUsedFiles)) return {};

    std::vector<char> spirvCode = _readSpirV(fileStream);

    //  Если чтение прошло удачно, переносим tmpUsedFiles в usedFiles
    if(!spirvCode.empty() && usedFiles != nullptr)
    {
      usedFiles->insert(tmpUsedFiles.begin(), tmpUsedFiles.end());
    }

    return spirvCode;
  }
  catch(std::exception& error)
  {
    Log::error() << "ShaderCache::load: " << error.what();
  }
  return {};
}

bool ShaderCache::_readAndCheckFileHeader(std::ifstream& fileStream) const
{
  char fileHeader[4]{};
  fileStream.read(fileHeader, sizeof(fileHeader));
  return fileHeader[0] == cacheHeader[0] &&
          fileHeader[1] == cacheHeader[1] &&
          fileHeader[2] == cacheHeader[2] &&
          fileHeader[3] == cacheHeader[3];
}

bool ShaderCache::_readAndCheckCompilatorVersion(
                                                std::ifstream& fileStream) const
{
  uint32_t version = 0;
  fileStream >> version;
  return version == ShaderCompilator::version;
}

bool ShaderCache::_readAndCheckFilename(
                                  std::ifstream& fileStream,
                                  const std::filesystem::path& shaderFile) const
{
  fs::path pathFromFile;
  fileStream >> pathFromFile;
  if(pathFromFile != shaderFile)
  {
    Log::warning() << "ShaderCache: hash conflict " << pathToUtf8(pathFromFile) << " " << pathToUtf8(shaderFile);
    return false;
  }
  return true;
}

bool ShaderCache::_readAndCheckStage(
                                  std::ifstream& fileStream,
                                  VkShaderStageFlagBits shaderStage,
                                  const std::filesystem::path& shaderFile) const
{
  uint32_t stageFromFile = 0;
  fileStream >> stageFromFile;
  if(stageFromFile != shaderStage)
  {
    Log::warning() << "ShaderCache: hash conflict on shader stage " << pathToUtf8(shaderFile);
    return false;
  }
  return true;
}

bool ShaderCache::_readAndCheckDefines(
                              std::ifstream& fileStream,
                              std::span<const ShaderCompilator::Define> defines,
                              const std::filesystem::path& shaderFile) const
{
  uint32_t definesCount = 0;
  fileStream >> definesCount;
  if(definesCount != defines.size())
  {
    Log::warning() << "ShaderCache: hash conflict on defines count" << pathToUtf8(shaderFile);
    return false;
  }

  for(uint32_t defineIndex = 0; defineIndex < definesCount; defineIndex++)
  {
    std::string name;
    fileStream >> name;
    std::string value;
    fileStream >> value;
    if(name != defines[defineIndex].name || value != defines[defineIndex].value)
    {
      Log::warning() << "ShaderCache: hash conflict on define. Shader:" << pathToUtf8(shaderFile) << " Cached define:" << name << " Required define:" << defines[defineIndex].name;
      return false;
    }
  }

  return true;
}

bool ShaderCache::_readAndCheckUsedFiles(
                                  std::ifstream& fileStream,
                                  std::unordered_set<fs::path>& targetSet) const
{
  ContentLoader& loader = ContentLoader::getLoader();

  uint32_t filesCount = 0;
  fileStream >> filesCount;

  for(uint32_t i = 0; i < filesCount; i++)
  {
    fs::path filename;
    fileStream >> filename;
    if(!loader.exists(filename)) return false;

    // Проверяем, что файл не изменился со времени компиляции шейдера
    time_t modificationTimeFromCache = 0;
    fileStream >> modificationTimeFromCache;

    fs::file_time_type latModification = loader.lastWriteTime(filename);
    auto ftime = std::chrono::clock_cast<std::chrono::system_clock>(
                                                              latModification);
    if(std::chrono::system_clock::to_time_t(ftime) != modificationTimeFromCache)
    {
      return false;
    }

    targetSet.insert(filename);
  }

  return true;
}

std::vector<char> ShaderCache::_readSpirV(std::ifstream& fileStream) const
{
  size_t codeSize = 0;
  fileStream >> codeSize;
  if(codeSize == 0) return {};

  std::vector<char> data(codeSize);
  fileStream.read(data.data(), 1);  //  Разделительный пробел перед spir-v кодом
  fileStream.read(data.data(), codeSize);

  return data;
}
