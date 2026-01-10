#pragma once

#include <asyncTask/AsyncTaskQueue.h>
#include <gui/AsyncTaskGUI.h>
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
    virtual void update() override;
    virtual void guiImplementation() override;

  private:
    AsyncTaskQueue _asyncTaskQueue;
    AsyncTaskGUI _asyncTaskGui;

    using TaskHandles =
                      std::vector<std::unique_ptr<AsyncTaskQueue::TaskHandle>>;
    TaskHandles _abortableHandles;
  };
}