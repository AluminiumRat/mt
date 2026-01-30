#include <Application.h>

#ifndef NDEBUG
  static constexpr bool enableDebug = true;
#else
  static constexpr bool enableDebug = false;
#endif

Application* Application::_instance = nullptr;

Application::Application() :
  _vkrLib("Texture processor",
          mt::VKRLib::AppVersion{.major = 0, .minor = 0, .patch = 0},
          VK_API_VERSION_1_3,
          enableDebug,
          enableDebug),
  _device(_guiLib.createDevice({}, {}, {}, mt::GRAPHICS_CONFIGURATION)),
  _asyncQueue(( [&](const mt::AsyncTaskQueue::Event& theEvent)
                {
                  _asyncTaskGui.addEvent(theEvent);
                })),
  _textureManager(_fileWatcher, _asyncQueue),
  _bufferManager(_fileWatcher, _asyncQueue),
  _techniqueManager(_fileWatcher, _asyncQueue),
  _textureViewerManager(&_techniqueManager)
{
  _instance = this;

  _guiLib.loadConfiguration("guiConfig.cfg");
}

Application::~Application() noexcept
{
  try
  {
    _guiLib.saveConfiguration("guiConfig.cfg");
  }
  catch(...)
  {
  }
}

void Application::run()
{
  while (!_guiLib.shouldBeClosed())
  {
    _asyncQueue.update();
    _fileWatcher.propagateChanges();
    _textureViewerManager.flush();

    _guiLib.updateWindows();
    _guiLib.drawWindows();
  }
}
