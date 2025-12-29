#pragma once

#include <memory>

#include <gui/GUIWindow.h>

#include <Project.h>
#include <TextureViewer.h>

class MainWindow : public mt::GUIWindow
{
public:
  MainWindow();
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator = (const MainWindow&) = delete;
  virtual ~MainWindow() noexcept = default;

protected:
  virtual void guiImplementation() override;
  virtual bool canClose() noexcept override;

private:
  void _processMainMenu();
  void _newProject();
  void _loadProject();
  void _saveIfNeeded();
  void _saveProject();
  void _saveProjectAs();
  void _updateTitle();
  void _runShader();
  void _showTextureViewer();

private:
  std::unique_ptr<Project> _project;

  //  Сохранение происходит синхронно в 1 кадре. Но для наглядности мы будем
  //  показывать окно сохранения в течении нескольких кадров. Это счетчик
  //  оставшихся кадров.
  int _saveWindowStage;

  TextureViewer _textureViewer;
};
