#pragma once

#include <util/Log.h>

namespace mt
{
  template <typename ReasonType>
  inline void Abort [[noreturn]] (ReasonType reason)
  {
    Log::error() << reason;
    abort();
  }
}