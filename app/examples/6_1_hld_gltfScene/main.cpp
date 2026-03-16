#include <exception>

#include <gui/GUILib.h>
#include <hld/HLDLib.h>
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
    guiLib.loadConfiguration("gltfSceneGui.cfg");

    HLDLib hldLib;

    std::unique_ptr<Device> device = guiLib.createDevice(
                                        { .accelerationStructure = {
                                              .accelerationStructure = VK_TRUE},
                                          .rayQuery = {.rayQuery = VK_TRUE}},
                                        { "VK_KHR_acceleration_structure",
                                          "VK_KHR_deferred_host_operations",
                                          "VK_KHR_ray_query"},
                                        /*{},
                                        {},*/
                                        GRAPHICS_CONFIGURATION);
    TestWindow mainWindow(*device);
    mainWindow.loadConfiguration();
    while (!guiLib.shouldBeClosed())
    {
      guiLib.updateWindows();
      guiLib.drawWindows();
    }

    guiLib.saveConfiguration("gltfSceneGui.cfg");

    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
