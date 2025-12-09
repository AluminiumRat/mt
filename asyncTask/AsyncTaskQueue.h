#pragma once

#include <atomic>
#include <functional>
#include <queue>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <asyncTask/AsyncTask.h>

namespace mt
{
  //  Класс управляет выполнением асинхронных задач (класс AsyncTask),
  //    организовывает их в очередь, следит за правильной последовательностью
  //    вызова методов класса AsyncTask
  class AsyncTaskQueue
  {
    //  Класс который позволяет остановить исполнение и дождаться окончания
    //    работы асинхронной задачи.
    //  В оснеовном необходим для того чтобы не оставлять работающих AsyncTask-ов
    //    после того как удалился класс, создавший их, и предотвратить обращение
    //    к нему из тасок.
    class TaskHandle
    {
    private:
      friend class AsyncTaskQueue;
      inline TaskHandle( AsyncTaskQueue& queue, AsyncTask& task) noexcept;
    public:
      TaskHandle(const TaskHandle&) = delete;
      TaskHandle& operator = (const TaskHandle&) = delete;
      //  ВНИМАНИЕ! Деструктор вызывает abortTask и может привести к вызову
      //    AsyncTask::restorePart, убедитесь что это потокобезопасно
      inline ~TaskHandle() noexcept;

      inline AsyncTask* task() const noexcept;

      //  Отправить таске команду на завершение и дождаться окончания
      //  её работы
      //  Внимание! этот метод может вызывать AsyncTask::restorePart, убедитесь
      //    что это потокобезопасно
      inline void abortTask() noexcept;

      //  Отключить хэндл от таски. Таска дальше живет сама по себе
      inline void releaseTask() noexcept;

    private:
      AsyncTaskQueue& _queue;
      std::atomic<AsyncTask*> _task;
    };

    enum EventType
    {
      TASK_ADDED_EVENT,
      STAGE_STARTED_EVENT,
      PERCENTS_DONE_EVENT,
      WARNING_EVENT,
      ERROR_EVENT,
      TASK_FINISHED_EVENT
    };

    struct Event
    {
      AsyncTask* task;
      EventType type;
      std::string description;  //  Для STAGE_STARTED_EVENT здесь название
                                //    стадии
                                //  Для WARNING_EVENT и ERROR_EVENT - текст
                                //    предупреждения или ошибки
                                //  Для других событий - пустая строка

      uint8_t percent;          //  Для PERCENTS_DONE_EVENT - обработанный
                                //    процент текущей стадии.
                                //  Для других событий - ноль
    };

  public:
    //  eventCallback - функтор, в который будут отсылаться события, связанные
    //    с асинхронными задачами. События накапливаются и передаются в
    //    калбэк во время вызова AsyncTaskQueue::update
    AsyncTaskQueue(const std::function<void(const Event&)> eventCallback);
    AsyncTaskQueue(const AsyncTaskQueue&) = delete;
    AsyncTaskQueue& operator = (const AsyncTaskQueue&) = delete;
    virtual ~AsyncTaskQueue() noexcept = default;

    //  Добавляем таску в очередь и забываем про неё
    void addTask(std::unique_ptr<AsyncTask> task);
    //  Добавляем таску в очередь и получаем хэндл, чтобы её можно было
    //    безопасно остановить.
    std::unique_ptr<TaskHandle> addManagedTask(std::unique_ptr<AsyncTask> task);

    //  Остановить таску и дождаться когда она завершится
    //  Внимание! этот метод вызывает AsyncTask::restorePart, убедитесь
    //    что это потокобезопасно
    void abortTask(TaskHandle& handle) noexcept;

    //  Вызывает синхронную часть у тасок, которые закончили асинхронную
    //    работу. Удаляет отработавшие таски.
    //  Необходимо периодически вызывать этот метод в синхронной части кода
    //    (скорее всего в главном цикле)
    void update();

  private:
    // Методы для вызова из AsyncTask
    friend class AsyncTask;
    void reportStage( AsyncTask& task, const char* stageName) noexcept;
    void reportPercent(AsyncTask& task, uint8_t percent) noexcept;
    void reportWarning( AsyncTask& task, const char* message) noexcept;

  private:
    //  Вызывается из TaskHandle
    void unregisterHandle(TaskHandle& stopper) noexcept;

  private:
    void _addEvent( AsyncTask* task,
                    EventType eventType,
                    const char* description,
                    uint8_t percent) noexcept;
    void _propagateEvent(const Event& theEvent);
    void _startTasks() noexcept;
    void _finishAsyncPart(AsyncTask& task) noexcept;
    void _invalidateHandle(AsyncTask& task) noexcept;

  private:
    using TaskQueue = std::queue<std::unique_ptr<AsyncTask>>;
    // Таски которые ещё не начали своё выполнение
    TaskQueue _waitQueue;

    using TaskList = std::vector<std::unique_ptr<AsyncTask>>;
    // Таски, которые выполняют асинхронную часть
    TaskList _inProgress;
    // Таски которые закончили асинхронную часть, но ещё не выполнили синхронную
    TaskQueue _finished;

    //  Таска с режимом EXCLUSIVE_MODE находится в асинхронной части,
    //  нельзя запускать другие таски на выполнение
    bool _exclusiveMode;

    //  Список хэндлов, чтобы сообщать им о том, что таска удалена
    using Handles = std::vector<TaskHandle*>;
    Handles _handles;

    using Events = std::vector<Event>;
    Events _events;
    std::function<void(const Event&)> _eventCallback;

    mutable std::recursive_mutex _mutex;
  };

  inline AsyncTaskQueue::TaskHandle::TaskHandle(
                                            AsyncTaskQueue& queue,
                                            AsyncTask& task) noexcept :
    _queue(queue),
    _task(&task)
  {
  }

  inline AsyncTaskQueue::TaskHandle::~TaskHandle() noexcept
  {
    abortTask();
    _queue.unregisterHandle(*this);
  }

  inline AsyncTask* AsyncTaskQueue::TaskHandle::task() const noexcept
  {
    return _task;
  }

  inline void AsyncTaskQueue::TaskHandle::abortTask() noexcept
  {
    _queue.abortTask(*this);
  }

  inline void AsyncTaskQueue::TaskHandle::releaseTask() noexcept
  {
    _task = nullptr;
  }
}