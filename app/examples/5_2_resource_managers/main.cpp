#include <exception>

#include <gui/GUILib.h>
#include <util/util.h>
#include <vkr/VKRLib.h>

#include <TestWindow.h>

using namespace mt;

int main(int argc, char* argv[])
{
  try
  {
    VKRLib vkrLib("VKTest",
                  VKRLib::AppVersion{.major = 0, .minor = 0, .patch = 0},
                  VK_API_VERSION_1_3,
                  true,
                  true);
    GUILib guiLib;

    std::unique_ptr<Device> device = guiLib.createDevice(
                                                      {},
                                                      {},
                                                      {},
                                                      GRAPHICS_CONFIGURATION);
    TestWindow mainWindow(*device);
    while (!guiLib.shouldBeClosed())
    {
      guiLib.updateWindows();
      guiLib.drawWindows();
    }

    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
