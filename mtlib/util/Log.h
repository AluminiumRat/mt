#pragma once

#include <iostream>
#include <mutex>
#include <time.h>

namespace mt
{
  class Log
  {
  public:
    static inline Log info();
    static inline Log warning();
    static inline Log error();

  private:
    inline Log();
  public:
    Log(const Log&) = delete;
    Log& operator = (const Log&) = delete;
    inline Log(Log&& other);
    inline Log& operator = (Log&& other);
    inline ~Log();

    template <typename DataType>
    inline Log& operator << (const DataType& data);

    static void setStream(std::ostream& stream);

  private:
    static std::mutex _writeMutex;
    static std::ostream* _stream;

    std::unique_lock<std::mutex> _mutexLock;
    bool _valid;
  };

  inline Log Log::info()
  {
    return std::move(Log() << "INFO: ");
  }

  inline Log Log::warning()
  {
    return std::move(Log() << "WARNING: ");
  }

  inline Log Log::error()
  {
    return std::move(Log() << "ERROR: ");
  }

  inline Log::Log() :
    _mutexLock(_writeMutex),
    _valid(true)
  {
    time_t currentTime = time(nullptr);
    tm timeinfo;
    localtime_s(&timeinfo, &currentTime);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", &timeinfo);

    (*_stream) << buffer << ": ";
  }

  inline Log::Log(Log&& other)
  {
    *this = std::move(other);
  }

  inline Log& Log::operator = (Log&& other)
  {
    _mutexLock = std::move(other._mutexLock);
    _valid = other._valid;
    other._valid = false;
    return *this;
  }

  inline Log::~Log()
  {
    if(_valid)
    {
      (*_stream)<<(std::endl);
      _stream->flush();
    }
  }

  template <typename DataType>
  inline Log& Log::operator << (const DataType& data)
  {
    (*_stream) << (data);
    return *this;
  }
}