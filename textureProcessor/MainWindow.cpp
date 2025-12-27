#include <stdexcept>
#include <fstream>
#include <memory>

#include <vulkan/vulkan.h>

#include <ddsSupport/ddsSupport.h>
#include <gui/ImGuiRAII.h>
#include <gui/modalDialogs.h>
#include <technique/TechniqueLoader.h>
#include <util/Log.h>
#include <vkr/image/ImageFormatFeatures.h>
#include <vkr/queue/CommandProducerGraphic.h>
#include <vkr/Device.h>
#include <MainWindow.h>

#include <Application.h>

// Сколько кадров будет показываться окно сохранения
#define SAVE_WINDOW_FRAME_COUNT 5

MainWindow::MainWindow() :
  GUIWindow(Application::instance().primaryDevice(), "Texture processor"),
  _saveWindowStage(0)
{
}

void MainWindow::guiImplementation()
{
  mt::GUIWindow::guiImplementation();

  _processMainMenu();
  if(_project != nullptr) _project->guiPass();

  // Если было сохранение, то на несколько кадров покажем окно сохранения
  if(_saveWindowStage != 0)
  {
    _saveWindowStage--;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(150, 70));
    mt::ImGuiWindow saveWindow("Save");
    ImGui::ProgressBar(0.5f);
  }

  Application::instance().asyncTaskGui().makeImGUI();
}

void MainWindow::_processMainMenu()
{
  //  Обработке шорткатов
  ImGuiIO& io = ImGui::GetIO();
  if(io.KeyCtrl)
  {
    if(ImGui::IsKeyPressed(ImGuiKey_O, false)) _loadProject();
    if(ImGui::IsKeyPressed(ImGuiKey_S, false)) _saveProject();
    if(ImGui::IsKeyPressed(ImGuiKey_R, false)) _runShader();
  }

  //  Собственно, само меню
  mt::ImGuiMainMenuBar mainMenu;
  if (!mainMenu.created()) return;

  mt::ImGuiMenu fileMenu("File");
  if(fileMenu.created())
  {
    if (ImGui::MenuItem("New")) _newProject();
    if (ImGui::MenuItem("Open", "Ctrl+O")) _loadProject();

    // Если проекта нет, то "Save" не должен быть активным
    if (_project == nullptr)
    {
      mt::ImGuiPushStyleColor pushColor(ImGuiCol_Text,
                                        ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
      if (ImGui::MenuItem("Save", "Ctrl+S")) _saveProject();
    }
    else if (ImGui::MenuItem("Save", "Ctrl+S")) _saveProject();

    fileMenu.end();
  }

  mt::ImGuiMenu buildMenu("Build");
  if (buildMenu.created())
  {
    if(ImGui::MenuItem("Run shader", "Ctrl+R")) _runShader();
    buildMenu.end();
  }
}

void MainWindow::_newProject()
{
  _saveIfNeeded();
  _project.reset(new Project(""));
  _updateTitle();
}

void MainWindow::_loadProject()
{
  _saveIfNeeded();

  std::filesystem::path file =
        mt::openFileDialog( this,
                            mt::FileFilters{{ .expression = "*.tpr",
                                              .description = "Project(*.tpr)"}},
                            "");
  if(!file.empty())
  {
    _project.reset(new Project(file));
    _updateTitle();
  }
}

void MainWindow::_saveIfNeeded()
{
  if(_project == nullptr) return;
  bool needSave = yesNoQuestionDialog(this,
                                      "New project",
                                      "Do you want to save project?",
                                      true);
  if(needSave) _saveProject();
}

void MainWindow::_saveProject()
{
  if(_project == nullptr) return;
  if(_project->projectFile().empty())
  {
    _saveProjectAs();
    return;
  }

  _project->save(_project->projectFile());
  _saveWindowStage = SAVE_WINDOW_FRAME_COUNT;
}

void MainWindow::_saveProjectAs()
{
  if (_project == nullptr) return;

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
    _saveWindowStage = SAVE_WINDOW_FRAME_COUNT;
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

void MainWindow::_runShader()
{
  if(_project == nullptr) throw std::runtime_error("Project is not opened");
  _project->runTechnique();
}

void MainWindow::drawImplementation(mt::CommandProducerGraphic& commandProducer,
                                    mt::FrameBuffer& frameBuffer)
{
  mt::CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                    frameBuffer);
  renderPass.endPass();

  GUIWindow::drawImplementation(commandProducer, frameBuffer);
}

bool MainWindow::canClose() noexcept
{
  try
  {
    if(_project == nullptr) return true;
    mt::QuestionButton button = yesNoCancelDialog(
                                                this,
                                                "Exit",
                                                "Do you want to save project?");
    if(button == mt::YES_BUTTON) _saveProject();
    if(button == mt::CANCEL_BUTTON) return false;
    return true;
  }
  catch(std::exception& error)
  {
    mt::Log::error() << error.what();
  }
  return true;
}
