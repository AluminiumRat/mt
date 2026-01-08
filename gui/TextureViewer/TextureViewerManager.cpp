#include <gui/textureViewer/TextureViewerManager.h>

using namespace mt;

//  Какое каличество кадров должен неиспользоваться виджет, чтобы он был удален
//  при вызове TextureViewerManager::flush
static constexpr int framesUnusedToDelete = 100;

TextureViewerManager* TextureViewerManager::_instance = nullptr;

TextureViewerManager::TextureViewerManager(TechniqueManager* techniqueManager) :
  _techniqueManager(techniqueManager)
{
  MT_ASSERT(_instance == nullptr);
  _instance = this;
}

TextureViewerManager::~TextureViewerManager() noexcept
{
  _instance = nullptr;
}

TextureViewer& TextureViewerManager::getOrCreateViewer( ImGuiID widgetId,
                                                        Device& device)
{
  WidgetKey key{widgetId , &device};

  WidgetsMap::iterator iWidget = _widgetsMap.find(key);
  if(iWidget != _widgetsMap.end())
  {
    iWidget->second.framesUnused = 0;
    return *iWidget->second.widget;
  }

  WidgedRecord newWidgetRecord;
  newWidgetRecord.framesUnused = 0;
  newWidgetRecord.widget.reset(new TextureViewer(device, _techniqueManager));
  TextureViewer& widgetRef = *newWidgetRecord.widget;

  _widgetsMap[key] = std::move(newWidgetRecord);

  return widgetRef;
}

void TextureViewerManager::flush() noexcept
{
  WidgetsMap::iterator iWidget = _widgetsMap.begin();
  while(iWidget != _widgetsMap.end())
  {
    WidgedRecord& widgetRecord = iWidget->second;
    if(widgetRecord.framesUnused < framesUnusedToDelete)
    {
      widgetRecord.framesUnused++;
      iWidget++;
    }
    else
    {
      iWidget = _widgetsMap.erase(iWidget);
    }
  }
}
