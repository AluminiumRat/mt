#pragma once

#include <deque>
#include <string>
#include <vector>

#include <gui/GUIWindow.h>
#include <resourceManagement/FileWatcher.h>

namespace mt
{
  class TestWindow :  public GUIWindow,
                      public FileObserver
  {
  public:
    TestWindow(Device& device, const char* name);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept;

    virtual void onFileChanged( const std::filesystem::path& filePath,
                                EventType eventType) override;

  protected:
    virtual void guiImplementation() override;

  private:
    void _fileListWindow();
    void _logWindow();
    void _addFile();
    void _deleteFile(const std::string& filename);
    void _addToLog(const std::string& text);

  private:
    std::vector<std::string> _watchedFiles;
    std::deque<std::string> _log;
  };
}