#include <algorithm>
#include <imgui.h>

#include <gui/ImGuiRAII.h>
#include <gui/modalDialogs.h>

#include <TestWindow.h>

namespace fs = std::filesystem;

using namespace mt;

TestWindow::TestWindow(Device& device, const char* name) :
  GUIWindow(device, name)
{
}

TestWindow::~TestWindow() noexcept
{
  FileWatcher::instance().removeObserver(*this);
}

void TestWindow::guiImplementation()
{
  {
    // Главное меню
    ImGuiMainMenuBar mainMenu;
    if(mainMenu.created())
    {
      if (ImGui::MenuItem("Add file")) _addFile();
    }
  }

  _fileListWindow();
  _logWindow();
}

void TestWindow::_fileListWindow()
{
  // Окно со списком контролируемых файлов
  ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(300, 530), ImGuiCond_Appearing);
  ImGuiWindow fileListWindow("File list");
  if(!fileListWindow.visible()) return;

  std::string deletedFile;    //  На случай, если пользователь захочет удалить
                              //  файл из списка,
  for(const std::string filename : _watchedFiles)
  {
    ImGui::Selectable(filename.c_str());
    ImGuiPopupContextItem popup;
    if (popup.created())
    {
      if(ImGui::Button("Delete"))
      {
        deletedFile = filename;
        ImGui::CloseCurrentPopup();
      }
    }
  }
  if (!deletedFile.empty()) _deleteFile(deletedFile);
}

void TestWindow::_logWindow()
{
  ImGui::SetNextWindowPos(ImVec2(350, 40), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(400, 530), ImGuiCond_Appearing);
  ImGuiWindow logWindow("Log");
  if(!logWindow.visible()) return;

  for (const std::string logString : _log)
  {
    ImGui::Text(logString.c_str());
  }
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

    if(FileWatcher::instance().addWatching(filePatch, *this))
    {
      _watchedFiles.push_back(filename);
      _addToLog(std::string("File has been added for watching: ") + filename);
    }
    else
    {
      _addToLog(std::string("File ") + filename + " is alrady watched");
    }
  }
}

void TestWindow::_deleteFile(const std::string& filename)
{
  fs::path file = fs::path((const char8_t*)filename.c_str());
  if (FileWatcher::instance().removeWatching(file, *this))
  {
    _addToLog(std::string("File has been removed from watching: " + filename));
  }

  std::vector<std::string>::iterator iFilename =
                std::find(_watchedFiles.begin(), _watchedFiles.end(), filename);
  MT_ASSERT(iFilename != _watchedFiles.end());
  _watchedFiles.erase(iFilename);
}

void TestWindow::onFileChanged( const fs::path& filePath,
                                EventType eventType)
{
  std::string filename = (const char*)filePath.u8string().c_str();

  switch (eventType)
  {
  case FILE_APPEARANCE:
    _addToLog(std::string("File found: " + filename));
    break;
  case FILE_DISAPPEARANCE:
    _addToLog(std::string("File lost: " + filename));
    break;
  case FILE_CHANGE:
    _addToLog(std::string("File changed: " + filename));
    break;
  }
}
