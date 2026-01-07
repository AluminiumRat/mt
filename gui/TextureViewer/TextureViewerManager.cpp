#include <gui/TextureViewer/TextureViewerManager.h>

using namespace mt;

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
    iWidget->second.wasUsed = true;
    return *iWidget->second.widget;
  }

  WidgedRecord newWidgetRecord;
  newWidgetRecord.wasUsed = true;
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
    if(widgetRecord.wasUsed)
    {
      widgetRecord.wasUsed = false;
      iWidget++;
    }
    else
    {
      iWidget = _widgetsMap.erase(iWidget);
    }
  }
}
