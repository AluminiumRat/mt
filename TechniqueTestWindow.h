#pragma once

#include <technique/Technique.h>
#include <util/Ref.h>
#include <GLFWRenderWindow.h>

namespace mt
{
  class TechniqueTestWindow : public GLFWRenderWindow
  {
  public:
    TechniqueTestWindow(Device& device);
    TechniqueTestWindow(const GLFWRenderWindow&) = delete;
    TechniqueTestWindow& operator = (const TechniqueTestWindow&) = delete;
    virtual ~TechniqueTestWindow() noexcept = default;

  private:
    Ref<Technique> _technique;
  };
}