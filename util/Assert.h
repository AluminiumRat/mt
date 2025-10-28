#pragma once

#include "util/Abort.h"

#ifdef _DEBUG
  #define MT_ASSERT(expression) if(!(expression)) {mt::Abort(#expression);}
#else
  #define MT_ASSERT(expression)
#endif