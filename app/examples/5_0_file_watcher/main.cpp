#include <exception>

#include <gui/GUILib.h>
#include <resourceManagement/FileWatcher.h>
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
    FileWatcher fileWatcher;
    GUILib guiLib;

    std::unique_ptr<Device> device = guiLib.createDevice(
                                                      {},
                                                      {},
                                                      {},
                                                      GRAPHICS_CONFIGURATION);
    TestWindow window(*device, "Test window");

    while (!guiLib.shouldBeClosed())
    {
      fileWatcher.propagateChanges();
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
