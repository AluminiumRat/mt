#include <algorithm>
#include <imgui.h>

#include <gui/fileDialog.h>

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
  std::string deletedFile;    //  На случай, если пользователь захочет удалить
                              //  файл из списка,
  ImGui::Begin("File list");
  for(const std::string filename : _watchedFiles)
  {
    ImGui::Selectable(filename.c_str());
    if (ImGui::BeginPopupContextItem())
    {
      if(ImGui::Button("Delete"))
      {
        deletedFile = filename;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
  ImGui::End();

  if(!deletedFile.empty()) _deleteFile(deletedFile);

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
