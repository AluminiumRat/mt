#include <stdexcept>

#include <Application.h>
#include <MainWindow.h>

int main(int argc, char* argv[])
{
  try
  {
    Application application;
    MainWindow window;
    window.loadConfiguration();
    application.run();
    return 0;
  }
  catch (std::exception& error)
  {
    std::cerr << error.what();
    return 1;
  }
}
