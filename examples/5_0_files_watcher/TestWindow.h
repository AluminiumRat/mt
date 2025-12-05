#pragma once

#include <deque>
#include <string>
#include <vector>

#include <gui/GUIWindow.h>

namespace mt
{
  class TestWindow : public GUIWindow
  {
  public:
    TestWindow(Device& device, const char* name);
    TestWindow(const TestWindow&) = delete;
    TestWindow& operator = (const TestWindow&) = delete;
    virtual ~TestWindow() noexcept = default;

  protected:
    virtual void guiImplementation() override;

  private:
    void _addFile();
    void _addToLog(const std::string& text);

  private:
    std::vector<std::string> _watchedFiles;
    std::deque<std::string> _log;
  };
}