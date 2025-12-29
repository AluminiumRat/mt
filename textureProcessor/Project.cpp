#include <fstream>
#include <stdexcept>

#include <imgui.h>

#include <yaml-cpp/yaml.h>

#include <asyncTask/AsyncTask.h>
#include <gui/GUIWindow.h>
#include <gui/ImGuiPropertyGrid.h>
#include <gui/ImGuiWidgets.h>
#include <gui/ImGuiRAII.h>
#include <gui/modalDialogs.h>
#include <util/Assert.h>
#include <util/fileSystemHelpers.h>
#include <vkr/image/ImageFormatFeatures.h>

#include <Application.h>
#include <BuildTextureTask.h>
#include <Project.h>

namespace fs = std::filesystem;

class Project::RebuildTechniqueTask : public mt::AsyncTask
{
public:
  RebuildTechniqueTask( Project& project,
                        mt::TechniqueConfigurator& configurator,
                        VkFormat targetFormat,
                        const fs::path& shaderFile) :
    AsyncTask("Compile shader", EXCLUSIVE_MODE, EXPLICIT),
    _project(project),
    _configurator(configurator),
    _shaderFile(shaderFile),
    _targetFormat(targetFormat)
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

    VkFormat colorAttachments[1] = { _targetFormat };
    mt::FrameBufferFormat fbFormat( colorAttachments,
                                    VK_FORMAT_UNDEFINED,
                                    VK_SAMPLE_COUNT_1_BIT);
    std::unique_ptr<mt::PassConfigurator> pass(
                                              new mt::PassConfigurator("Pass"));
    pass->setFrameBufferFormat(&fbFormat);
    pass->setShaders(shaders);
    pass->setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    _configurator.addPass(std::move(pass));

    _configurator.addGUIHint({"intrinsic.mipLevel",
                              mt::TechniqueConfiguration::GUI_HINT_HIDDEN});
    _configurator.addGUIHint({"intrinsic.arrayIndex",
                              mt::TechniqueConfiguration::GUI_HINT_HIDDEN});

    _configurator.rebuildConfiguration(&_usedFiles);
  }

  virtual void finalizePart() override
  {
    _configurator.propogateConfiguration();
    _addFilesToWatching();
    reportInfo("Build succeeded");
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
      for(const fs::path& file : _usedFiles)
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
  fs::path _shaderFile;
  VkFormat _targetFormat;
  std::unordered_set<fs::path> _usedFiles;
};

Project::Project( const fs::path& file) :
  _projectFile(file),
  _imageFormat(VK_FORMAT_B8G8R8A8_SRGB),
  _outputSize(256, 256),
  _mipsCount(1),
  _arraySize(1),
  _configurator(new mt::TechniqueConfigurator(
                                        Application::instance().primaryDevice(),
                                        "Make texture technique")),
  _technique(new mt::Technique(*_configurator)),
  _propsGrid( *_technique,
                mt::TechniquePropertyGridCommon{
                        &Application::instance().textureManager(),
                        &Application::instance().bufferManager(),
                        Application::instance().primaryDevice().graphicQueue(),
                        &_projectFile})
{
  if(!_projectFile.empty())
  {
    _load();
    rebuildTechnique();
  }
}

void Project::_load()
{
  fs::path projectFolder = _projectFile.parent_path();

  std::ifstream fileStream(_projectFile, std::ios::binary);
  if (!fileStream) throw std::runtime_error(std::string("file not found: ") + (const char*)_projectFile.u8string().c_str());

  YAML::Node rootNode = YAML::Load(fileStream);
  if (!rootNode.IsMap()) throw std::runtime_error(std::string("wrong file format: ") + (const char*)_projectFile.u8string().c_str());

  {
    std::string shaderFilename = rootNode["shader"].as<std::string>("");
    _shaderFile = mt::utf8ToPath(shaderFilename);
    _shaderFile = mt::restoreAbsolutePath(_shaderFile, projectFolder);
  }

  {
    std::string outputFilename = rootNode["output"].as<std::string>("");
    _outputFile = mt::utf8ToPath(outputFilename);
    _outputFile = mt::restoreAbsolutePath(_outputFile, projectFolder);
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

  YAML::Node shaderPropsNode = rootNode["shaderProps"];
  _propsGrid.load(shaderPropsNode);
}

Project::~Project() noexcept
{
  Application::instance().fileWatcher().removeObserver(*this);
}

void Project::save(const fs::path& file)
{
  fs::path projectFolder = file.parent_path();

  // Создаем YAML разметку
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "shader";
  out << YAML::Value << mt::pathToUtf8(mt::makeStoredPath(_shaderFile,
                                                          projectFolder));

  out << YAML::Key << "output";
  out << YAML::Value << mt::pathToUtf8(mt::makeStoredPath(_outputFile,
                                                          projectFolder));

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

  out << YAML::Key << "shaderProps";
  out << YAML::Value;
  _propsGrid.save(out);

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
  std::unique_ptr<mt::AsyncTask> rebuildTask(
                                      new RebuildTechniqueTask( *this,
                                                                *_configurator,
                                                                _imageFormat,
                                                                _shaderFile));
  _rebuildTaskHandle = Application::instance().asyncQueue().addManagedTask(
                                                        std::move(rebuildTask));
}

void Project::runTechnique()
{
  if(_technique->configuration() == nullptr) throw std::runtime_error("The shader wasn't compiled correct");
  if(!_technique->isReady()) throw std::runtime_error("Not all resources are setted");

  if (_textureTaskHandle != nullptr) _textureTaskHandle->abortTask();
  std::unique_ptr<mt::AsyncTask> newTask( new BuildTextureTask( *_technique,
                                                                _outputFile,
                                                                _imageFormat,
                                                                _outputSize,
                                                                _mipsCount,
                                                                _arraySize,
                                                                *this));
  _textureTaskHandle = Application::instance().asyncQueue().addManagedTask(
                                                            std::move(newTask));
}

void Project::onFileChanged(const fs::path&,
                            EventType eventType)
{
  if(eventType != FILE_DISAPPEARANCE) rebuildTechnique();
}

void Project::guiPass()
{
  mt::ImGuiWindow window("Project");

  ImGui::Text("Shader:");
  ImGui::SameLine();
  if(mt::fileSelectionLine("Shader:", _shaderFile)) _selectShader();

  ImGui::SeparatorText("Output");
  _guiOutputProps();

  ImGui::SeparatorText("Shader properties");
  _propsGrid.makeGUI();
}

void Project::_selectShader() noexcept
{
  try
  {
    fs::path file =
        mt::openFileDialog(
                  mt::GUIWindow::currentWindow(),
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
    mt::errorDialog(mt::GUIWindow::currentWindow(), "Error", "Unable to open shader file");
  }
}

bool formatSelectionLine(VkFormat &format)
{
  static const mt::Bimap<VkFormat> formatsMap{
    "Output formats",
    {
      {VK_FORMAT_B8G8R8A8_SRGB, "B8G8R8A8_SRGB"},
      {VK_FORMAT_R32G32B32A32_SFLOAT, "R32G32B32A32_SFLOAT"},
      {VK_FORMAT_B10G11R11_UFLOAT_PACK32, "B10G11R11_UFLOAT"}
    }};
  return mt::enumSelectionCombo("##format", format, formatsMap);
}

void Project::_guiOutputProps()
{
  mt::ImGuiPropertyGrid outputPropsGrid("##outputProps");
  outputPropsGrid.addRow("File:");
  if(mt::fileSelectionLine("File:", _outputFile)) _selectOutputFile();

  outputPropsGrid.addRow("Format:");
  if(formatSelectionLine(_imageFormat)) rebuildTechnique();

  outputPropsGrid.addRow("Size:");
  int sizeValues[] = { _outputSize.x, _outputSize.y };
  ImGui::InputInt2("##size", sizeValues, 0);
  _outputSize = glm::ivec2(sizeValues[0], sizeValues[1]);
  _outputSize = glm::clamp(_outputSize, 1, 16536);

  outputPropsGrid.addRow("Mips:");
  ImGui::InputInt("##mips", &_mipsCount, 0);
  _mipsCount = glm::clamp(_mipsCount, 1, 1024);

  outputPropsGrid.addRow("Array size:");
  ImGui::InputInt("##arraySize", &_arraySize, 0);
  _arraySize = glm::clamp(_arraySize, 1, 16536);
}

void Project::_selectOutputFile() noexcept
{
  try
  {
    fs::path file =
        mt::saveFileDialog(
                          mt::GUIWindow::currentWindow(),
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
