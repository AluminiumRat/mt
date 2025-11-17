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

  protected:
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;

  private:
    Ref<Technique> _technique;
    Selection& _selector1;
    Selection& _selector2;
  };
}