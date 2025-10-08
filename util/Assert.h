#pragma once

#include "util/Abort.h"

#define MT_ASSERT(expression) if(!(expression)) {mt::Abort(#expression);}