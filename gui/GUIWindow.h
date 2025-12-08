#pragma once

#include <imgui.h>

#include <gui/RenderWindow.h>

namespace mt
{
  //  Рутины по привязке Dear ImGUI к движку
  //  ВНИМАНИЕ!!! Класс не потокобезопасный, активно использует переключение
  //    ImGUI контекста. Используйте все окна только из основного потока.
  //    Если в многопоточной части используются ImGUI, то убедитесь, что эта
  //    часть не пересекается по времени с апдейтом и рендером оконной системы.
  class GUIWindow : public RenderWindow
  {
  public:
    GUIWindow(Device& device, const char* name);
    GUIWindow(const GUIWindow&) = delete;
    GUIWindow& operator = (const GUIWindow&) = delete;
    virtual ~GUIWindow() noexcept;

    virtual void update() override;

  protected:
    //  Здесь должне находится сам код, генерирующий виджеты для текущего кадра
    //  Переопределите и заполните этот метод в потомках
    virtual void guiImplementation();

  protected:
    virtual void onClose() noexcept override;
    virtual void drawImplementation(CommandProducerGraphic& commandProducer,
                                    FrameBuffer& frameBuffer) override;
    virtual void applyConfiguration(const WindowConfiguration& configuration);
    virtual void fillConfiguration(
                            WindowConfiguration& configuration) const override;

  private:
    void _clean() noexcept;

  private:
    ImGuiContext* _imguiContext;
  };
}