#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device) :
  RenderWindow( device,
                "Test window",
                std::nullopt,
                std::nullopt,
                VK_FORMAT_UNDEFINED)
{
}

void TestWindow::drawImplementation(FrameBuffer& frameBuffer)
{
}
