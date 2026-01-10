#pragma once

#include "util/Abort.h"

#ifndef NDEBUG
  #define MT_ASSERT(expression) if(!(expression)) {mt::Abort(#expression);}
#else
  #define MT_ASSERT(expression)
#endif