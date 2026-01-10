#include <exception>

#include <vkr/dumpHardware.h>
#include <vkr/VKRLib.h>

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

    dumpHardware( true,       // dumpLayers
                  true,       // dumpExtensions
                  true,       // dumpDevices
                  true,       // dumpQueues
                  true,       // dumpMemory
                  nullptr);
    return 0;
  }
  catch (std::exception& error)
  {
    Log::error() << error.what();
    return 1;
  }
}
