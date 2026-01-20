#include <thread>

#include <imgui.h>

#include <gui/ImGuiRAII.h>

#include <TestWindow.h>

using namespace mt;

class MyTask : public AsyncTask
{
public:
  MyTask( const char* name,
          ExecutionMode executionMode,
          Visibility visibility,
          bool makeStages,
          bool makeWarning,
          bool makeError) :
    AsyncTask(name, executionMode, visibility),
    _makeStages(makeStages),
    _makeWarning(makeWarning),
    _makeError(makeError)
  {
  }

protected:
  virtual void asyncPart() override
  {
    if(_makeStages) reportStage("Stage1");

    for(int step = 1; step <= 10; step++)
    {
      if(shouldBeAborted()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      reportPercent((uint8_t)(step * 5));
    }

    if(_makeWarning) reportWarning("Some warning");
    if(_makeError) throw std::runtime_error("Some error");

    for (int step = 1; step <= 10; step++)
    {
      if (shouldBeAborted()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      reportPercent((uint8_t)(50 + step * 5));
    }

    if(!_makeStages) return;
    reportStage("Stage2");
    for (int step = 1; step <= 10; step++)
    {
      if (shouldBeAborted()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      reportPercent((uint8_t)(step * 10));
    }
  }

private:
  bool _makeStages;
  bool _makeWarning;
  bool _makeError;
};

TestWindow::TestWindow(Device& device, const char* name) :
  GUIWindow(device, name, std::nullopt, std::nullopt, VK_FORMAT_UNDEFINED),
  _asyncTaskQueue([&](const AsyncTaskQueue::Event& theEvent)
                  {
                    _asyncTaskGui.addEvent(theEvent);
                  })
{
}

void TestWindow::update()
{
  _asyncTaskQueue.update();
  GUIWindow::update();
}

std::string taskName(const char* prefix, int index)
{
  return std::string(prefix) + std::to_string(index);
}

void TestWindow::guiImplementation()
{
  static int taksCreated = 0;
  static bool makeStages = false;
  static bool makeError = false;
  static bool makeWarning = false;
  static bool abortable = true;
  static bool hidden = false;

  ImGui::SetNextWindowSize(ImVec2(0, 0));
  ImGuiWindow window("Control");

  ImGui::Checkbox("Stages", &makeStages);
  ImGui::Checkbox("Make error", &makeError);
  ImGui::Checkbox("Make warning", &makeWarning);
  ImGui::Checkbox("Abortable", &abortable);
  ImGui::Checkbox("Hidden", &hidden);

  AsyncTask::Visibility visibility = hidden ? AsyncTask::SILENT :
                                              AsyncTask::EXPLICIT;

  if(ImGui::Button("Add regular task"))
  {
    std::unique_ptr<AsyncTask> newTask(new MyTask(
                                    taskName("Regular", taksCreated).c_str(),
                                    AsyncTask::PARALLEL_MODE,
                                    visibility,
                                    makeStages,
                                    makeWarning,
                                    makeError));
    std::unique_ptr<AsyncTaskQueue::TaskHandle> handle = 
                            _asyncTaskQueue.addManagedTask(std::move(newTask));
    if(abortable) _abortableHandles.push_back(std::move(handle));
    else handle->releaseTask();
    taksCreated++;
  }

  if (ImGui::Button("Add exclusive task"))
  {
    std::unique_ptr<AsyncTask> newTask(new MyTask(
                                    taskName("Exclusive", taksCreated).c_str(),
                                    AsyncTask::EXCLUSIVE_MODE,
                                    visibility,
                                    makeStages,
                                    makeWarning,
                                    makeError));
    std::unique_ptr<AsyncTaskQueue::TaskHandle> handle =
                            _asyncTaskQueue.addManagedTask(std::move(newTask));
    if (abortable) _abortableHandles.push_back(std::move(handle));
    else handle->releaseTask();
    taksCreated++;
  }

  if (ImGui::Button("Abort")) _abortableHandles.clear();

  _asyncTaskGui.makeImGUI();
}
