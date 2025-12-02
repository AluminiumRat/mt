#include <imgui.h>

#include <TestWindow.h>

using namespace mt;

TestWindow::TestWindow(Device& device, const char* name) :
  GUIWindow(device, name)
{
}

void TestWindow::guiImplementation()
{
  ImGui::Begin("Hello, world!");
  ImGui::Text("This is some useful text.");
  ImGui::End();
}
