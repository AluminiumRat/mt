#pragma once

#include <gui/GUIWindow.h>

class MainWindow : public mt::GUIWindow
{
public:
  MainWindow();
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator = (const MainWindow&) = delete;
  virtual ~MainWindow() noexcept = default;

protected:
  virtual void guiImplementation() override;
  virtual void drawImplementation(mt::CommandProducerGraphic& commandProducer,
                                  mt::FrameBuffer& frameBuffer) override;

private:
  void _processMainMenu();
  void _newProject();
  void _loadProject();
  void _saveIfNeeded();
  void _saveProject();
  void _saveProjectAs();
};
