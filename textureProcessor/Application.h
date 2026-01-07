#pragma once

#include <memory>

#include <asyncTask/AsyncTaskQueue.h>
#include <gui/TextureViewer/TextureViewerManager.h>
#include <gui/AsyncTaskGUI.h>
#include <gui/GUILib.h>
#include <resourceManagement/BufferResourceManager.h>
#include <resourceManagement/FileWatcher.h>
#include <resourceManagement/TextureManager.h>
#include <resourceManagement/TechniqueManager.h>
#include <util/Assert.h>
#include <vkr/Device.h>
#include <vkr/VKRLib.h>

class Application
{
public:
  Application();
  Application(const Application&) = delete;
  Application& operator = (const Application&) = delete;
  virtual ~Application() noexcept;

  void run();

  inline static Application& instance() noexcept;

  inline mt::Device& primaryDevice() noexcept;

  inline mt::AsyncTaskQueue& asyncQueue() noexcept;
  inline mt::FileWatcher& fileWatcher() noexcept;
  inline mt::TextureManager& textureManager() noexcept;
  inline mt::BufferResourceManager& bufferManager() noexcept;
  inline mt::TechniqueManager& techniqueManager() noexcept;
  inline mt::AsyncTaskGUI& asyncTaskGui() noexcept;

private:
  static Application* _instance;

  mt::VKRLib _vkrLib;
  mt::GUILib _guiLib;

  std::unique_ptr<mt::Device> _device;

  mt::AsyncTaskQueue _asyncQueue;
  mt::FileWatcher _fileWatcher;
  mt::TextureManager _textureManager;
  mt::BufferResourceManager _bufferManager;
  mt::TechniqueManager _techniqueManager;
  mt::TextureViewerManager _textureViewerManager;
  mt::AsyncTaskGUI _asyncTaskGui;
};

inline Application& Application::instance() noexcept
{
  MT_ASSERT(_instance != nullptr);
  return *_instance;
}

inline mt::Device& Application::primaryDevice() noexcept
{
  return *_device;
}

inline mt::AsyncTaskQueue& Application::asyncQueue() noexcept
{
  return _asyncQueue;
}

inline mt::FileWatcher& Application::fileWatcher() noexcept
{
  return _fileWatcher;
}

inline mt::TextureManager& Application::textureManager() noexcept
{
  return _textureManager;
}

inline mt::BufferResourceManager& Application::bufferManager() noexcept
{
  return _bufferManager;
}

inline mt::TechniqueManager& Application::techniqueManager() noexcept
{
  return _techniqueManager;
}

inline mt::AsyncTaskGUI& Application::asyncTaskGui() noexcept
{
  return _asyncTaskGui;
}
