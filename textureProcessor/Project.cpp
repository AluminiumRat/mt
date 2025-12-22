#include <fstream>
#include <stdexcept>

#include <imgui.h>

#include <yaml-cpp/yaml.h>

#include <asyncTask/AsyncTask.h>
#include <gui/modalDialogs.h>
#include <util/Assert.h>
#include <vkr/image/ImageFormatFeatures.h>

#include <Application.h>
#include <Project.h>

class Project::RebuildTechniqueTask : public mt::AsyncTask
{
public:
  RebuildTechniqueTask( Project& project,
                        mt::TechniqueConfigurator& configurator,
                        const std::filesystem::path& shaderFile) :
    AsyncTask("Compile shader", EXCLUSIVE_MODE, EXPLICIT),
    _project(project),
    _configurator(configurator),
    _shaderFile(shaderFile)
  {
  }

  virtual void asyncPart() override
  {
    _configurator.clear();

    mt::PassConfigurator::ShaderInfo shaders[2] =
                                { { .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                    .file = "textureProcessor/fsQuad.vert"},
                                  { .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                    .file = _shaderFile}};

    VkFormat colorAttachments[1] = { VK_FORMAT_R32G32B32A32_SFLOAT };
    mt::FrameBufferFormat fbFormat( colorAttachments,
                                    VK_FORMAT_UNDEFINED,
                                    VK_SAMPLE_COUNT_1_BIT);
    std::unique_ptr<mt::PassConfigurator> pass(
                                              new mt::PassConfigurator("Pass"));
    pass->setFrameBufferFormat(&fbFormat);
    pass->setShaders(shaders);

    _configurator.addPass(std::move(pass));

    _configurator.rebuildConfiguration(&_usedFiles);
  }

  virtual void finalizePart() override
  {
    _configurator.propogateConfiguration();
    _addFilesToWatching();
  }

  virtual void restorePart() noexcept override
  {
    _addFilesToWatching();
  }

private:
  void _addFilesToWatching() noexcept
  {
    Application::instance().fileWatcher().removeObserver(_project);

    try
    {
      for(const std::filesystem::path& file : _usedFiles)
      {
        Application::instance().fileWatcher().addWatching(file, _project);
      }
    }
    catch(std::exception& error)
    {
      mt::Log::error() << error.what();
    }
  }

private:
  Project& _project;
  mt::TechniqueConfigurator& _configurator;
  std::filesystem::path _shaderFile;
  std::unordered_set<std::filesystem::path> _usedFiles;
};

//  Перевести pathToSave в формат, пригодный для сохранения в файле проекта
//  Если pathToSave - это абсолютный путь и pathToSave лежит в projectFolder
//    или её подпапке, то возвращает путь относительно projectFolder, иначе
//    возвращает исходный pathToSave
std::filesystem::path makeStoredPath(
                                    const std::filesystem::path& pathToSave,
                                    const std::filesystem::path& projectFolder)
{
  if(pathToSave.empty()) return pathToSave;
  if (!pathToSave.is_absolute()) return pathToSave;
  MT_ASSERT(projectFolder.is_absolute());

  std::filesystem::path normalizedPath = pathToSave.lexically_normal();
  std::filesystem::path projectFolderNorm = projectFolder.lexically_normal();

  auto [mismatchInFile, mismatchInFolder] =
                                        std::mismatch(normalizedPath.begin(),
                                                      normalizedPath.end(),
                                                      projectFolderNorm.begin(),
                                                      projectFolderNorm.end());
  //  Проверяем, а лежит ли файл в указанной папке
  if(mismatchInFolder != projectFolderNorm.end()) return pathToSave;

  //  Восстанавливаем относительный путь от точки расхождения
  std::filesystem::path storedPath;
  for(std::filesystem::path::const_iterator iPath = mismatchInFile;
      iPath != normalizedPath.end();
      iPath++)
  {
    storedPath = storedPath  / *iPath;
  }
  return storedPath;
}

std::string pathToUtf8(const std::filesystem::path& path)
{
  return (const char*)path.u8string().c_str();
}

//  Восстановить путь для файла из сохраненного в файле проекта
std::filesystem::path restoreAbsolutePath(
                                    const std::filesystem::path& storedPath,
                                    const std::filesystem::path& projectFolder)
{
  if (storedPath.empty()) return storedPath;
  if (storedPath.is_absolute()) return storedPath;
  MT_ASSERT(projectFolder.is_absolute());
  return projectFolder / storedPath;
}

std::filesystem::path utf8ToPath(const std::string& filename)
{
  return std::filesystem::path((char8_t*)filename.c_str());
}

Project::Project( const std::filesystem::path& file,
                  const mt::BaseWindow& parentWindow) :
  _projectFile(file),
  _parentWindow(parentWindow),
  _imageFormat(VK_FORMAT_B8G8R8A8_SRGB),
  _outputSize(256, 256),
  _mipsCount(1),
  _arraySize(1),
  _configurator(new mt::TechniqueConfigurator(
                                        Application::instance().primaryDevice(),
                                        "Make texture technique")),
  _technique(new mt::Technique(*_configurator))
{
  if(!_projectFile.empty())
  {
    _load();
    rebuildTechnique();
  }
}

void Project::_load()
{
  std::filesystem::path projectFolder = _projectFile.parent_path();

  std::ifstream fileStream(_projectFile, std::ios::binary);
  if (!fileStream) throw std::runtime_error(std::string("file not found: ") + (const char*)_projectFile.u8string().c_str());

  YAML::Node rootNode = YAML::Load(fileStream);
  if (!rootNode.IsMap()) throw std::runtime_error(std::string("wrong file format: ") + (const char*)_projectFile.u8string().c_str());

  {
    std::string shaderFilename = rootNode["shader"].as<std::string>("");
    _shaderFile = utf8ToPath(shaderFilename);
    _shaderFile = restoreAbsolutePath(_shaderFile, projectFolder);
  }

  {
    std::string outputFilename = rootNode["output"].as<std::string>("");
    _outputFile = utf8ToPath(outputFilename);
    _outputFile = restoreAbsolutePath(_outputFile, projectFolder);
  }

  {
    std::string outputFormatName = rootNode["outputFormat"].as<std::string>("");
    if(!outputFormatName.empty())
    {
      const mt::ImageFormatFeatures& formatDescription =
                                        mt::getFormatFeatures(outputFormatName);
      _imageFormat = formatDescription.format;
    }
  }

  _outputSize.x = rootNode["outputWidth"].as<int32_t>(_outputSize.x);
  _outputSize.y = rootNode["outputHeight"].as<int32_t>(_outputSize.y);
  _outputSize = glm::clamp(_outputSize, 1, 16536);

  _mipsCount = rootNode["outputMips"].as<int32_t>(_mipsCount);
  _mipsCount = glm::clamp(_mipsCount, 1, 1024);

  _arraySize = rootNode["outputArraySize"].as<int32_t>(_arraySize);
  _arraySize = glm::clamp(_arraySize, 1, 16536);
}

Project::~Project() noexcept
{
  Application::instance().fileWatcher().removeObserver(*this);
}

void Project::save(const std::filesystem::path& file)
{
  std::filesystem::path projectFolder = _projectFile.parent_path();

  // Создаем YAML разметку
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "shader";
  out << YAML::Value << pathToUtf8(makeStoredPath(_shaderFile, projectFolder));

  out << YAML::Key << "output";
  out << YAML::Value << pathToUtf8(makeStoredPath(_outputFile, projectFolder));

  const mt::ImageFormatFeatures& formatDescription =
                                            mt::getFormatFeatures(_imageFormat);
  out << YAML::Key << "outputFormat";
  out << YAML::Value << formatDescription.name;

  out << YAML::Key << "outputWidth";
  out << YAML::Value << _outputSize.x;
  out << YAML::Key << "outputHeight";
  out << YAML::Value << _outputSize.y;

  out << YAML::Key << "outputMips";
  out << YAML::Value << _mipsCount;

  out << YAML::Key << "outputArraySize";
  out << YAML::Value << _arraySize;

  out << YAML::EndMap;

  // А теперь сохраняем это всё в файл
  std::ofstream fileStream(file, std::ios::binary);
  if (!fileStream.is_open())
  {
    throw std::runtime_error(std::string("Unable to open file ") + (const char*)file.u8string().c_str());
  }
  fileStream.write(out.c_str(), out.size());

  _projectFile = file;
}

void Project::rebuildTechnique()
{
  if(_rebuildTaskHandle != nullptr) _rebuildTaskHandle->abortTask();
  std::unique_ptr<mt::AsyncTask> loadingTask(
                                      new RebuildTechniqueTask( *this,
                                                                *_configurator,
                                                                _shaderFile));
  _rebuildTaskHandle = Application::instance().asyncQueue().addManagedTask(
                                                        std::move(loadingTask));
}

void Project::onFileChanged(const std::filesystem::path& filePath,
                            EventType eventType)
{
  if(eventType != FILE_DISAPPEARANCE) rebuildTechnique();
}

bool fileSelectionLine( const char* controlId,
                        const std::filesystem::path& filePath) noexcept
{
  ImGui::PushID(controlId);
  bool pressed =  ImGui::SmallButton("...");
  ImGui::SameLine();
  ImGui::Text(pathToUtf8(filePath.filename()).c_str());
  ImGui::SetItemTooltip(pathToUtf8(filePath).c_str());
  ImGui::PopID();
  return pressed;
}

VkFormat formatSelectionLine(VkFormat currentFormat)
{
  struct FormatRecord
  {
    const char* name;
    VkFormat format;
  };
  static FormatRecord formats[] = {
                      {"B8G8R8A8_SRGB",VK_FORMAT_B8G8R8A8_SRGB},
                      {"R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT}};

  //  Ищем надпись для превью
  const char* previewText = "none";
  for(const FormatRecord& formatRecord : formats)
  {
    if(currentFormat == formatRecord.format)
    {
      previewText = formatRecord.name;
      break;
    }
  }

  //  Собственно, сам комбо бокс
  if(ImGui::BeginCombo( "##format", previewText, 0))
  {
    for(const FormatRecord& formatRecord : formats)
    {
      if(ImGui::Selectable( formatRecord.name,
                            currentFormat == formatRecord.format))
      {
        currentFormat = formatRecord.format;
      }
    }
    ImGui::EndCombo();
  }
  return currentFormat;
}

void Project::guiPass()
{
  if(!ImGui::Begin("Project")) return;

  ImGui::Text("Shader:");
  ImGui::SameLine();
  if(fileSelectionLine("Shader:", _shaderFile)) _selectShader();

  ImGui::SeparatorText("Output");
  _guiOutputProps();

  ImGui::End();
}

void Project::_selectShader() noexcept
{
  try
  {
    std::filesystem::path file =
        mt::openFileDialog( &_parentWindow,
                    mt::FileFilters{{ .expression = "*.frag",
                                      .description = "Fragment shader(*.frag)"}},
                    "");
    if(!file.empty())
    {
      _shaderFile = file;
      rebuildTechnique();
    }
  }
  catch (std::exception& error)
  {
    mt::Log::error() << error.what();
    mt::errorDialog(&_parentWindow, "Error", "Unable to open shader file");
  }
}

void Project::_guiOutputProps() noexcept
{
  if(!ImGui::BeginTable("##outputProps", 2, 0)) return;
  ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(1);
  ImGui::PushItemWidth(-FLT_MIN);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("File:");
  ImGui::TableSetColumnIndex(1);
  if (fileSelectionLine("File:", _outputFile)) _selectOutputFile();

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Format:");
  ImGui::TableSetColumnIndex(1);
  _imageFormat = formatSelectionLine(_imageFormat);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Size:");
  ImGui::TableSetColumnIndex(1);
  int sizeValues[] = { _outputSize.x, _outputSize.y };
  ImGui::InputInt2("##size", sizeValues, 0);
  _outputSize = glm::ivec2(sizeValues[0], sizeValues[1]);
  _outputSize = glm::clamp(_outputSize, 1, 16536);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Mips:");
  ImGui::TableSetColumnIndex(1);
  ImGui::InputInt("##mips", &_mipsCount, 0);
  _mipsCount = glm::clamp(_mipsCount, 1, 1024);

  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("Array size:");
  ImGui::TableSetColumnIndex(1);
  ImGui::InputInt("##arraySize", &_arraySize, 0);
  _arraySize = glm::clamp(_arraySize, 1, 16536);

  ImGui::EndTable();
}

void Project::_selectOutputFile() noexcept
{
  try
  {
    std::filesystem::path file =
        mt::saveFileDialog( &_parentWindow,
                    mt::FileFilters{{ .expression = "*.dds",
                                      .description = "DDS image(*.dds)"}},
                    "");
    if(!file.empty())
    {
      if (!file.has_extension()) file.replace_extension("dds");
      _outputFile = file;
    }
  }
  catch (std::exception& error)
  {
    mt::Log::error() << error.what();
  }
}
