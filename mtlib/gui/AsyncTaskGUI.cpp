#include <imgui.h>

#include <gui/AsyncTaskGUI.h>
#include <gui/ImGuiRAII.h>
#include <util/Assert.h>

using namespace mt;

static constexpr size_t maxMessagesInQueue = 200;

AsyncTaskGUI::AsyncTaskGUI() :
  _explicitTasksCount(0),
  _showMessagesWindow(false)
{
}

AsyncTaskGUI::Tasks::iterator AsyncTaskGUI::_getTaskInfo(
                                                      AsyncTask& task) noexcept
{
  for(Tasks::iterator iTask = _tasks.begin();
      iTask != _tasks.end();
      iTask++)
  {
    if((*iTask)->task == &task) return iTask;
  }
  return _tasks.end();
}

void AsyncTaskGUI::addEvent(const AsyncTaskQueue::Event& theEvent)
{
  switch(theEvent.type)
  {
  case AsyncTaskQueue::TASK_ADDED_EVENT:
  {
    MT_ASSERT(_getTaskInfo(*theEvent.task) == _tasks.end());
    _tasks.push_back(std::make_unique<TaskInfo>());
    _tasks.back()->task = theEvent.task;
    if(theEvent.task->visibility() == AsyncTask::EXPLICIT)
    {
      _explicitTasksCount++;
    }
    break;
  }

  case AsyncTaskQueue::TASK_FINISHED_EVENT:
  {
    Tasks::iterator iTask = _getTaskInfo(*theEvent.task);
    MT_ASSERT(iTask != _tasks.end());
    _tasks.erase(iTask);
    if (theEvent.task->visibility() == AsyncTask::EXPLICIT)
    {
      _explicitTasksCount--;
    }
    break;
  }

  case AsyncTaskQueue::STAGE_STARTED_EVENT:
  {
    Tasks::iterator iTask = _getTaskInfo(*theEvent.task);
    MT_ASSERT(iTask != _tasks.end());
    (*iTask)->currentStage = theEvent.description;
    break;
  }

  case AsyncTaskQueue::PERCENTS_DONE_EVENT:
  {
    Tasks::iterator iTask = _getTaskInfo(*theEvent.task);
    MT_ASSERT(iTask != _tasks.end());
    (*iTask)->percentsDone = theEvent.percent;
    break;
  }

  case AsyncTaskQueue::INFO_EVENT:
  {
    _messages.emplace_back();
    _messages.back().type = INFO_MESSAGE;
    _messages.back().taskName = theEvent.task->name();
    _messages.back().message = theEvent.description;
    if(_messages.size() > maxMessagesInQueue) _messages.pop_front();
    break;
  }

  case AsyncTaskQueue::WARNING_EVENT:
  {
    _messages.emplace_back();
    _messages.back().type = WARNING_MESSAGE;
    _messages.back().taskName = theEvent.task->name();
    _messages.back().message = theEvent.description;
    if(_messages.size() > maxMessagesInQueue) _messages.pop_front();
    _showMessagesWindow = true;
    break;
  }

  case AsyncTaskQueue::ERROR_EVENT:
  {
    _messages.emplace_back();
    _messages.back().type = ERROR_MESSAGE;
    _messages.back().taskName = theEvent.task->name();
    _messages.back().message = theEvent.description;
    if(_messages.size() > maxMessagesInQueue) _messages.pop_front();
    _showMessagesWindow = true;
    break;
  }
  }
}

void AsyncTaskGUI::makeImGUI()
{
  if(_explicitTasksCount != 0) _makeExplicitTasksWindow();
  if(_showMessagesWindow) _makeMessagesWindow();
}

void AsyncTaskGUI::_makeExplicitTasksWindow()
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(500, 400));
  ImGui::SetNextWindowSize(ImVec2(0, 0));

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGuiWindow progressWindow("Progress");
  for(const std::unique_ptr<TaskInfo>& taskInfo : _tasks)
  {
    if(taskInfo->task->visibility() != AsyncTask::EXPLICIT) continue;

    ImGui::SeparatorText(taskInfo->task->name().c_str());
    if(!taskInfo->currentStage.empty())
    {
      ImGui::Text("Stage:");
      ImGui::SameLine();
      ImGui::Text(taskInfo->currentStage.c_str());
    }
    ImGui::ProgressBar((float)taskInfo->percentsDone / 100.0f);
  }
}

void AsyncTaskGUI::_makeMessagesWindow()
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(350, 200), ImVec2(4096, 4096));

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  ImGuiWindow messagesWindow("Messages", &_showMessagesWindow);
  if(!_showMessagesWindow) _messages.clear();

  for(Messages::reverse_iterator iMessage = _messages.rbegin();
      iMessage != _messages.rend();
      iMessage++)
  {
    const Message& message = *iMessage;
    ImGui::SeparatorText(message.taskName.c_str());
    if(message.type == INFO_MESSAGE)
    {
      ImGuiPushStyleColor pushColor(ImGuiCol_Text, IM_COL32(50, 255, 50, 255));
      ImGui::Text("INFO: ");
    }
    else if(message.type == WARNING_MESSAGE)
    {
      ImGuiPushStyleColor pushColor(ImGuiCol_Text,
                                    IM_COL32(255, 255, 50, 255));
      ImGui::Text("WARNING: ");
    }
    else
    {
      ImGuiPushStyleColor pushColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
      ImGui::Text("ERROR: ");
    }
    ImGui::SameLine();
    ImGui::Text(message.message.c_str());
  }
}
