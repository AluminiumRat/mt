#pragma once

#include <memory>

#include <gui/GUIWindow.h>

#include <Project.h>

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
  void _newProject() noexcept;
  void _loadProject() noexcept;
  void _saveIfNeeded() noexcept;
  void _saveProject() noexcept;
  void _saveProjectAs() noexcept;
  void _updateTitle();

private:
  std::unique_ptr<Project> _project;
};
