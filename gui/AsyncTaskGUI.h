#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <AsyncTask/AsyncTaskQueue.h>

namespace mt
{
  //  Класс принимает события от AsyncTaskQueue, отслеживает состояние
  //  тасок и создает ImGUI объекты для их визуализации
  class AsyncTaskGUI
  {
  public:
    AsyncTaskGUI();
    AsyncTaskGUI(const AsyncTaskGUI&) = delete;
    AsyncTaskGUI& operator = (const AsyncTaskGUI&) = delete;
    ~AsyncTaskGUI() = default;

    //  В этот метод необходимо передавать все события от AsyncTaskQueue
    void addEvent(const AsyncTaskQueue::Event& theEvent);

    //  Создать объекты ImGUI. Вызывается каждый кадр в GUI обходе (например,
    //    в GUIWindow::guiImplementation)
    void makeImGUI();

  private:
    struct TaskInfo
    {
      AsyncTask* task = nullptr;
      std::string currentStage;
      uint8_t percentsDone = 0;
    };
    using Tasks = std::vector<std::unique_ptr<TaskInfo>>;

    enum MessageType
    {
      INFO_MESSAGE,
      WARNING_MESSAGE,
      ERROR_MESSAGE
    };
    struct Message
    {
      MessageType type;
      std::string taskName;
      std::string message;
    };
    using Messages = std::list<Message>;

  private:
    Tasks::iterator _getTaskInfo(AsyncTask& task) noexcept;
    void _makeExplicitTasksWindow();
    void _makeMessagesWindow();

  private:
    Tasks _tasks;
    size_t _explicitTasksCount;
    bool _showMessagesWindow;

    Messages _messages;
  };
}