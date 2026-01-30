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
    guiLib.loadConfiguration("ImguiDemo.cfg");

    std::unique_ptr<Device> device = guiLib.createDevice(
                                                      {},
                                                      {},
                                                      {},
                                                      GRAPHICS_CONFIGURATION);
    TestWindow window1(*device, "Test window1");
    window1.loadConfiguration();

    TestWindow window2(*device, "Test window2");
    window2.loadConfiguration();

    while (!guiLib.shouldBeClosed())
    {
      guiLib.updateWindows();
      guiLib.drawWindows();
    }

    guiLib.saveConfiguration("ImguiDemo.cfg");

    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
