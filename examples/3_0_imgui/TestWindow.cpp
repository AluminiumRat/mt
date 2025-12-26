#include <stdexcept>

#include <imgui.h>

#include <gui/ImGuiRAII.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device, const char* name) :
  GUIWindow(device, name)
{
}

void TestWindow::guiImplementation()
{
  ImGuiWindow window("Hello, world!");
  if(window.visible())
  {
    ImGui::Text("This is some useful text.");
    if(ImGui::Button("Throw an exception")) throw std::runtime_error("Something is wrong!");
  }
}
