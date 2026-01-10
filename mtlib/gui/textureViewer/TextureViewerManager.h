#pragma once

#include <memory>
#include <unordered_map>

#include <imgui.h>

#include <gui/textureViewer/TextureViewer.h>
#include <util/Ref.h>
#include <util/Assert.h>

namespace mt
{
  class TechniqueManager;
  class Device;

  //  Центральный класс-синглтон, который создает, хранит и очищает объекты
  //    TextureViewer для работы в immediately mode стиле (метод
  //    TextureViewer::makeGUIIm).
  //  Если вы не используете immediately mode, то объект этого класса не
  //    обязательно создавать. В противном случае он должен быть создан до
  //    первого вызова TextureViewer::makeGUIIm
  class TextureViewerManager
  {
  public:
    //  Если techniqueManager не nullptr, то она будет использована для
    //    загрузки техники отрисовки виджета(асинхронно). В противном случае
    //    техника будет загруженаться синхронно для каждого виджета.
    TextureViewerManager(TechniqueManager* techniqueManager);
    TextureViewerManager(const TextureViewerManager&) = delete;
    TextureViewerManager& operator = (const TextureViewerManager&) = delete;
    ~TextureViewerManager() noexcept;

    inline static TextureViewerManager& instance() noexcept;

    TextureViewer& getOrCreateViewer(ImGuiID widgetId, Device& device);

    //  Очистить инстансы, которые давное не были использованы
    //  Необходимо вызывать периодически(можно раз в кадр) для предотвращения
    //    утечек ресурсов
    void flush() noexcept;

  private:
    struct WidgedRecord
    {
      std::unique_ptr<TextureViewer> widget;
      // Счетчик, сколько кадров виджет не использовался
      int framesUnused;

      WidgedRecord() noexcept = default;
      WidgedRecord(const WidgedRecord&) = delete;
      WidgedRecord(WidgedRecord&&) noexcept = default;
      WidgedRecord& operator = (const WidgedRecord&) = delete;
      WidgedRecord& operator = (WidgedRecord&&) noexcept = default;
      ~WidgedRecord() noexcept = default;
    };
    //  Ключ для поиска виджета в мапе виджетов
    struct WidgetKey
    {
      ImGuiID widgetId;
      Device* device;

      std::strong_ordering operator <=> (
                                    const WidgetKey&) const noexcept = default;
    };
    struct KeyHash
    {
      std::size_t operator()(const WidgetKey& key) const
      {
        return std::hash<ImGuiID>()(key.widgetId) ^
                 std::hash<Device*>()(key.device);
      }
    };
    using WidgetsMap = std::unordered_map<WidgetKey, WidgedRecord, KeyHash>;

  private:
    static TextureViewerManager* _instance;
    TechniqueManager* _techniqueManager;
    WidgetsMap _widgetsMap;
  };

  inline TextureViewerManager& TextureViewerManager::instance() noexcept
  {
    MT_ASSERT(_instance != nullptr);
    return *_instance;
  }
}