#pragma once

#include <hld/FrameContext.h>

class GlobalLight;

namespace mt
{
  struct ColorFrameContext : public FrameContext
  {
    const GlobalLight* illumination;
  };
}