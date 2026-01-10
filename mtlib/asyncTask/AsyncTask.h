#pragma once

#include <atomic>
#include <future>
#include <string>

namespace mt
{
  class AsyncTaskQueue;

  //  Абстрактная задача, предназначенная для выполнения в отдельном потоке
  //  Работает в паре с AsyncTaskQueue
  class AsyncTask
  {
  public:
    enum ExecutionMode
    {
      //  Задача выполняется в отдельном потоке, но не может параллелиться с
      //    другими задачами (в пределах одной AsyncTaskQueue)
      //  Используется, когда между несколькими задачами есть зависимости и
      //    задачи необходимо разделить по времени
      EXCLUSIVE_MODE,
      //  Задачи с режимом PARALLEL_MODE могут выполняться одновременно
      //    в нескольких параллельных потоках.
      PARALLEL_MODE
    };

    enum Visibility
    {
      //  Информация об выполнении задачи показывается пользователю (если
      //    такой механизм вообще предусмотрен в приложении)
      EXPLICIT,
      //  Задача выполняется незаметно для пользователя без вывода в GUI
      SILENT
    };

  public:
    AsyncTask(const char* name,
              ExecutionMode executionMode,
              Visibility visibility);
    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator = (const AsyncTask&) = delete;
    virtual ~AsyncTask() noexcept = default;

    inline const std::string& name() const noexcept;
    inline ExecutionMode executionMode() const noexcept;
    inline Visibility visibility() const noexcept;

  protected:
    //  Таска должна быть прекращена
    inline bool shouldBeAborted() const noexcept;

  protected:
    //  Асинхронная часть таски, выполняется в отдельном потоке
    virtual void asyncPart();
    //  Синхронная часть таски.
    //  Выполняется только в том случае, если make не сгенерировал исключений и
    //    таска не была прекращена через AsyncTaskQueue::abort
    //  Выполняется в основном потоке в AsyncTaskQueue::update
    virtual void finalizePart();
    //  Аварийный обработчик. Выполняется, если make или finalize выбросили
    //  исключение, либо если был вызван AsyncTaskQueue::abort
    //  Выполняется в основном потоке в AsyncTaskQueue::update либо в
    //  TaskHandle::abortTask
    virtual void restorePart() noexcept;

  protected:
    //  Сообщить всем желающим (скорее всего в GUI или лог), что таска
    //    перешла к выполнению новой стадии
    //  Также вызывает внутри себя reportPercent
    void reportStage(const char* stageName) noexcept;
    //  Сообщить всем желающим (скорее всего в GUI или лог), что выполнена
    //    некоторая часть работы (в процентах)
    void reportPercent(uint8_t percent) noexcept;
    //  Сообщить всем желающим (скорее всего в GUI или лог), какое-то тексторое
    //    сообщение
    void reportInfo(const char* message) noexcept;
    //  Сообщить всем желающим (скорее всего в GUI или лог), предупреждение
    //  Вызов этого метода не прекращает работу таски.
    //  Если произошла ошибка, после которой таска не может быть продолжена,
    //    генерируйте std::runtime_exception, либо вызывайте mt::Abort
    void reportWarning(const char* message) noexcept;

  private:
    //  Методы для вызова из AsyncTaskQueue
    friend class AsyncTaskQueue;
    inline void setQueue(AsyncTaskQueue& queue) noexcept;
    inline void setFuture(std::future<void>&& future) noexcept;
    inline std::future<void>& future() noexcept;
    //  Асинхронная часть
    void make();
    //  Синхронная часть
    void finalize();
    //  Обработчик на случай исключений или отмены таски
    void restoreState() noexcept;
    //  Сообщить таске, что её необходимо остановить
    inline void notifyAbort() noexcept;

  private:
    std::string _name;
    ExecutionMode _executionMode;
    Visibility _visibility;
    AsyncTaskQueue* _queue;
    size_t _lastPercent;

    std::atomic<bool> _shouldBeAborted;

    //  На этом фьюче будем ждать таску и получать исключение
    std::future<void> _future;
  };

  inline const std::string& AsyncTask::name() const noexcept
  {
    return _name;
  }

  inline AsyncTask::ExecutionMode AsyncTask::executionMode() const noexcept
  {
    return _executionMode;
  }

  inline AsyncTask::Visibility AsyncTask::visibility() const noexcept
  {
    return _visibility;
  }

  inline void AsyncTask::setQueue(AsyncTaskQueue& queue) noexcept
  {
    _queue = &queue;
  }

  inline void AsyncTask::setFuture(std::future<void>&& future) noexcept
  {
    _future = std::move(future);
  }

  inline std::future<void>& AsyncTask::future() noexcept
  {
    return _future;
  }

  inline void AsyncTask::notifyAbort() noexcept
  {
    _shouldBeAborted.store(true);
  }

  inline bool AsyncTask::shouldBeAborted() const noexcept
  {
    return _shouldBeAborted.load();
  }
}