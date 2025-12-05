#include <imgui.h>

#include <gui/fileDialog.h>

#include <TestWindow.h>

namespace fs = std::filesystem;

using namespace mt;

TestWindow::TestWindow(Device& device, const char* name) :
  GUIWindow(device, name)
{
}

void TestWindow::guiImplementation()
{
  ImGui::BeginMainMenuBar();
  if(ImGui::MenuItem("Add file")) _addFile();
  ImGui::EndMainMenuBar();

  static bool firstLaunch = true;

  // Окно со списком контролируемых файлов
  if(firstLaunch)
  {
    ImGui::SetNextWindowPos(ImVec2(20, 40));
    ImGui::SetNextWindowSize(ImVec2(300, 530));
  }
  ImGui::Begin("File list");
  for(const std::string filename : _watchedFiles)
  {
    ImGui::Text(filename.c_str());
  }
  ImGui::End();

  // Окно лога
  if (firstLaunch)
  {
    ImGui::SetNextWindowPos(ImVec2(350, 40));
    ImGui::SetNextWindowSize(ImVec2(400, 530));
  }
  ImGui::Begin("Log");
  for (const std::string logString : _log)
  {
    ImGui::Text(logString.c_str());
  }
  ImGui::End();

  firstLaunch = false;
}

void TestWindow::_addToLog(const std::string& text)
{
  _log.push_back(text);
  if(_log.size() > 20) _log.pop_front();
}

void TestWindow::_addFile()
{
  fs::path filePatch = openFileDialog(
      this,
      FileFilters { {.expression = "*.*", .description = "All files(*.*)"}},
      "");

  if(!filePatch.empty())
  {
    std::string filename = (const char*)filePatch.u8string().c_str();
    _watchedFiles.push_back(filename);
    _addToLog(std::string("File was added: ") + filename);
  }
}