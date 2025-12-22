#include <fstream>
#include <memory>

#include <vulkan/vulkan.h>

#include <ddsSupport/ddsSupport.h>
#include <gui/modalDialogs.h>
#include <technique/TechniqueLoader.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <MainWindow.h>

#include <Application.h>

MainWindow::MainWindow() :
  GUIWindow(Application::instance().primaryDevice(), "Texture processor")
{
}

void MainWindow::guiImplementation()
{
  mt::GUIWindow::guiImplementation();

  _processMainMenu();
  if(_project != nullptr) _project->guiPass();

  Application::instance().asyncTaskGui().makeImGUI();
}

void MainWindow::_processMainMenu()
{
  if(!ImGui::BeginMainMenuBar()) return;

  if(ImGui::BeginMenu("File"))
  {
    if (ImGui::MenuItem("New")) _newProject();
    if (ImGui::MenuItem("Open")) _loadProject();

    // Если проекта нет, то "Save" и "Save as" должны быть неактивными
    if (_project == nullptr)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    }
    if (ImGui::MenuItem("Save")) _saveProject();
    if (ImGui::MenuItem("Save as...")) _saveProjectAs();
    if (_project == nullptr) ImGui::PopStyleColor();

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void MainWindow::_newProject() noexcept
{
  try
  {
    _saveIfNeeded();
    _project.reset(new Project("", *this));
    _updateTitle();
  }
  catch(std::exception& error)
  {
    mt::Log::error() << error.what();
    mt::errorDialog(this, "New", "Unable to create new project");
  }
}

void MainWindow::_loadProject() noexcept
{
  _saveIfNeeded();

  try
  {
    std::filesystem::path file =
        mt::openFileDialog( this,
                            mt::FileFilters{{ .expression = "*.tpr",
                                              .description = "Project(*.tpr)"}},
                            "");
    if(!file.empty())
    {
      _project.reset(new Project(file, *this));
      _updateTitle();
    }
  }
  catch (std::exception& error)
  {
    mt::Log::error() << error.what();
    mt::errorDialog(this, "Open", "Unable to open project");
  }
}

void MainWindow::_saveIfNeeded() noexcept
{
  if(_project == nullptr) return;
  bool needSave = yesNoQuestionDialog(this,
                                      "New project",
                                      "Do you want to save project?",
                                      true);
  if(needSave) _saveProject();
}

void MainWindow::_saveProject() noexcept
{
  if(_project == nullptr) return;
  if(_project->projectFile().empty())
  {
    _saveProjectAs();
    return;
  }

  try
  {
    _project->save(_project->projectFile());
  }
  catch (std::exception& error)
  {
    mt::Log::error() << error.what();
    mt::errorDialog(this, "Save", "Unable to save project");
  }
}

void MainWindow::_saveProjectAs() noexcept
{
  if (_project == nullptr) return;

  try
  {
    std::filesystem::path file =
        mt::saveFileDialog( this,
                            mt::FileFilters{{ .expression = "*.tpr",
                                              .description = "Project(*.tpr)"}},
                            "");
    if(!file.empty())
    {
      if (!file.has_extension()) file.replace_extension("tpr");
      _project->save(file);
      _updateTitle();
    }
  }
  catch (std::exception& error)
  {
    mt::Log::error() << error.what();
    mt::errorDialog(this, "Save", "Unable to save project");
  }
}

void MainWindow::_updateTitle()
{
  std::string title = "Texture processor";
  if(_project != nullptr && !_project->projectFile().empty())
  {
    title = title + " : " +
            (const char*)_project->projectFile().filename().u8string().c_str();
  }
  setWindowTitle(title.c_str());
}

void MainWindow::drawImplementation(mt::CommandProducerGraphic& commandProducer,
                                    mt::FrameBuffer& frameBuffer)
{
  mt::CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                    frameBuffer);
  renderPass.endPass();

  GUIWindow::drawImplementation(commandProducer, frameBuffer);
}
