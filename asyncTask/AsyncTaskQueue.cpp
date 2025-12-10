#include <algorithm>
#include <stdexcept>

#include <asyncTask/AsyncTaskQueue.h>
#include <util/Abort.h>
#include <util/Assert.h>
#include <util/Log.h>

using namespace mt;

AsyncTaskQueue::AsyncTaskQueue(
                        const std::function<void(const Event&)> eventCallback) :
  _exclusiveMode(false),
  _eventCallback(eventCallback)
{
}

AsyncTaskQueue::~AsyncTaskQueue() noexcept
{
  // Раздаем всем запущенным таскам команду на аборт
  std::vector<AsyncTask*> tasksToWait;
  {
    std::lock_guard lock(_mutex);
    for(const std::unique_ptr<AsyncTask>& task : _inProgress)
    {
      task->notifyAbort();
      tasksToWait.push_back(task.get());
    }
  }

  // Ждем когда таски закончат выполнение
  for(AsyncTask* task : tasksToWait)
  {
    MT_ASSERT(task->future().valid());
    task->future().wait();
  }
}

void AsyncTaskQueue::reportStage( AsyncTask& task,
                                  const char* stageName) noexcept
{
  if(!_eventCallback) return;

  std::lock_guard lock(_mutex);
  _addEvent(&task, STAGE_STARTED_EVENT, stageName, 0);
}

void AsyncTaskQueue::reportPercent( AsyncTask& task, uint8_t percent) noexcept
{
  if (!_eventCallback) return;

  std::lock_guard lock(_mutex);
  _addEvent(&task, PERCENTS_DONE_EVENT, "", percent);
}

void AsyncTaskQueue::reportWarning( AsyncTask& task,
                                    const char* message) noexcept
{
  if (!_eventCallback) return;

  std::lock_guard lock(_mutex);
  _addEvent(&task, WARNING_EVENT, message, 0);
}

void AsyncTaskQueue::_addEvent( AsyncTask* task,
                                EventType eventType,
                                const char* description,
                                uint8_t percent) noexcept
{
  try
  {
    _events.emplace_back(Event{ .task = task,
                                .type = eventType,
                                .description = description,
                                .percent = percent });
  }
  catch(std::exception& error)
  {
    Log::error() << "AsyncTaskQueue:: Unable to add event: " << error.what();
  }
}

void AsyncTaskQueue::addTask(std::unique_ptr<AsyncTask> task)
{
  MT_ASSERT(task != nullptr);
    AsyncTask* taskPtr = task.get();

  std::lock_guard lock(_mutex);

  _waitQueue.push(std::move(task));
  _addEvent(taskPtr, TASK_ADDED_EVENT, "", 0);

  _startTasks();
}

std::unique_ptr<AsyncTaskQueue::TaskHandle>
                              AsyncTaskQueue::addManagedTask(
                                                std::unique_ptr<AsyncTask> task)
{
  MT_ASSERT(task != nullptr);
  AsyncTask* taskPtr = task.get();

  std::lock_guard lock(_mutex);
  std::unique_ptr<TaskHandle> handle(new TaskHandle(*this, *task));
  _handles.push_back(handle.get());
  try
  {
    _waitQueue.push(std::move(task));
    _addEvent(taskPtr, TASK_ADDED_EVENT, "", 0);
    _startTasks();
  }
  catch(...)
  {
    _handles.pop_back();
    throw;
  }

  return handle;
}

void AsyncTaskQueue::_startTasks() noexcept
{
  try
  {
    while(!_waitQueue.empty() && !_exclusiveMode)
    {
      //  Нельзя добавлять эксклюзивную таску, если какие-то таски уже
      //  выполняются
      if( _waitQueue.front()->executionMode() == AsyncTask::EXCLUSIVE_MODE &&
          !_inProgress.empty())
      {
        break;
      }

      std::unique_ptr<AsyncTask> task = std::move(_waitQueue.front());
      _waitQueue.pop();

      if(task->executionMode() == AsyncTask::EXCLUSIVE_MODE)
      {
        _exclusiveMode = true;
      }

      AsyncTask* taskPtr = task.get();
      auto executor = [&, taskPtr]()
      {
        try
        {
          taskPtr->make();
        }
        catch(...)
        {
          _finishAsyncPart(*taskPtr);
          throw;
        }
        _finishAsyncPart(*taskPtr);
      };

      task->setQueue(*this);
      _inProgress.push_back(std::move(task));
      taskPtr->setFuture(std::async(std::launch::async, executor));
    }
  }
  catch (...)
  {
    Abort("AsyncTaskQueue::_startTasks: unable to start task.");
  }
}

void AsyncTaskQueue::_finishAsyncPart(AsyncTask& task) noexcept
{
  try
  {
    std::lock_guard lock(_mutex);

    if (task.executionMode() == AsyncTask::EXCLUSIVE_MODE)
    {
      _exclusiveMode = false;
    }

    // Переносим таску в список _finished
    for(TaskList::iterator iTask = _inProgress.begin();
        iTask != _inProgress.end();
        iTask++)
    {
      if(iTask->get() == &task)
      {
        std::unique_ptr<AsyncTask> taskLock = std::move(*iTask);
        _inProgress.erase(iTask);
        _finished.push(std::move(taskLock));
        break;
      }
    }

    _startTasks();
  }
  catch (std::exception& error)
  {
    Log::error() << "AsyncTaskQueue: " << error.what();
    Abort("AsyncTaskQueue::_finishAsyncPart: unable to finish async part.");
  }
}

void AsyncTaskQueue::abortTask(TaskHandle& handle) noexcept
{
  try
  {
    std::lock_guard lock(_mutex);

    AsyncTask* task = handle.task();
    if(task == nullptr) return;

    handle.releaseTask();

    task->notifyAbort();
    if(task->future().valid()) task->future().wait();

    task->restoreState();
  }
  catch (std::exception& error)
  {
    Log::error() << "AsyncTaskQueue::abortTask: " << error.what();
    Abort("AsyncTaskQueue::abortTask: unable to wait async task.");
  }
}

void AsyncTaskQueue::unregisterHandle(TaskHandle& handle) noexcept
{
  std::lock_guard lock(_mutex);

  Handles::iterator iHandle = std::find(_handles.begin(),
                                        _handles.end(),
                                        &handle);
  if(iHandle != _handles.end()) _handles.erase(iHandle);
}

void AsyncTaskQueue::_invalidateHandle(AsyncTask& task) noexcept
{
  for(TaskHandle* handle : _handles)
  {
    if(handle->task() == &task)
    {
      handle->releaseTask();
      return;
    }
  }
}

void AsyncTaskQueue::update()
{
  for(const Event& theEvent : _events) _propagateEvent(theEvent);
  _events.clear();

  while(true)
  {
    std::unique_ptr<AsyncTask> task;
    {
      std::lock_guard lock(_mutex);

      if(_finished.empty()) break;

      task = std::move(_finished.front());
      _finished.pop();

      _invalidateHandle(*task);
    }

    if(!task->shouldBeAborted())
    {
      MT_ASSERT(task->future().valid())
      try
      {
        task->future().get();
        task->finalize();
      }
      catch(std::exception& error)
      {
        Log::error() << task->name() << " : " << error.what();
        if (_eventCallback) _propagateEvent(Event{.task = task.get(),
                                                  .type = ERROR_EVENT,
                                                  .description = error.what(),
                                                  .percent = 0});
        task->restoreState();
      }
    }
    if (_eventCallback) _propagateEvent(Event{.task = task.get(),
                                              .type = TASK_FINISHED_EVENT,
                                              .description = "",
                                              .percent = 0 });
  }
}

void AsyncTaskQueue::_propagateEvent(const Event& theEvent)
{
  if(!_eventCallback) return;

  try
  {
    _eventCallback(theEvent);
  }
  catch(std::exception& error)
  {
    Log::error() << "AsyncTaskQueue: unable to propagate event: " << error.what();
  }
}
