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

  Application::instance().asyncTaskGui().makeImGUI();
}

void MainWindow::_processMainMenu()
{
  if(!ImGui::BeginMainMenuBar()) return;

  if(ImGui::BeginMenu("File"))
  {
    if (ImGui::MenuItem("New")) _newProject();
    if (ImGui::MenuItem("Open")) _loadProject();
    if (ImGui::MenuItem("Save")) _saveProject();
    if (ImGui::MenuItem("Save as...")) _saveProjectAs();
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void MainWindow::_newProject()
{
  _saveIfNeeded();
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
  }
}

void MainWindow::_saveIfNeeded()
{
  bool needSave = yesNoQuestionDialog(this,
                                      "New project",
                                      "Do you want to save project?",
                                      true);
  if(needSave) _saveProject();
}

void MainWindow::_saveProject()
{
}

void MainWindow::_saveProjectAs()
{
  std::filesystem::path file =
        mt::saveFileDialog( this,
                            mt::FileFilters{{ .expression = "*.tpr",
                                              .description = "Project(*.tpr)"}},
                            "");
  if(!file.empty())
  {
    if (!file.has_extension()) file.replace_extension("tpr");
  }
}

void MainWindow::drawImplementation(mt::CommandProducerGraphic& commandProducer,
                                    mt::FrameBuffer& frameBuffer)
{
  mt::CommandProducerGraphic::RenderPass renderPass(commandProducer,
                                                    frameBuffer);
  renderPass.endPass();

  GUIWindow::drawImplementation(commandProducer, frameBuffer);
}
