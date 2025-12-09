#include <asyncTask/AsyncTask.h>
#include <asyncTask/AsyncTaskQueue.h>

using namespace mt;

AsyncTask::AsyncTask( const char* name,
                      ExecutionMode executionMode,
                      Visibility visibility) :
  _name(name),
  _executionMode(executionMode),
  _visibility(visibility),
  _queue(nullptr),
  _lastPercent(0),
  _shouldBeAborted(false)
{
}

void AsyncTask::make()
{
  if(shouldBeAborted()) return;
  asyncPart();
}

void AsyncTask::asyncPart()
{
}

void AsyncTask::finalize()
{
  if(shouldBeAborted()) return;
  finalizePart();
}

void AsyncTask::finalizePart()
{
}

void AsyncTask::restoreState() noexcept
{
  restorePart();
}

void AsyncTask::restorePart() noexcept
{
}

void AsyncTask::reportStage(const char* stageName) noexcept
{
  if(_queue == nullptr) return;
  _queue->reportStage(*this, stageName);
  reportPercent(0);
}

void AsyncTask::reportPercent(uint8_t percent) noexcept
{
  if(_queue == nullptr) return;
  if(_lastPercent == percent) return;
  _lastPercent = percent;
  _queue->reportPercent(*this, percent);
}

void AsyncTask::reportWarning(const char* message) noexcept
{
  if(_queue == nullptr) return;
  _queue->reportWarning(*this, message);
}
