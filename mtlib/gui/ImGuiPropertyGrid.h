#pragma once

#include <imgui.h>

namespace mt
{
  //  Хэлпер для построения проперти грида и одновременно RAII для
  //    таблицы, которая используется для грида
  //  В конструкторе создает таблицу с 2 столбцами и настраивает выравнивание
  //  В деструкторе финализирует таблицу
  class ImGuiPropertyGrid
  {
  public:
    inline ImGuiPropertyGrid(const char* gridName);
    inline ~ImGuiPropertyGrid() noexcept;

    //  Добавляет в грид(таблицу) новую строку,в первый столбец кладет надпись
    //    label и переходит во второй столбец, где должен находиться
    //    пользовательский виджет
    //  label не может быть равен nullptr
    //  toolTip может быть nullptr
    inline void addRow(const char* label, const char* toolTip = nullptr);

    //  Завершает таблицу
    //  Можно не вызывать каждый раз, так как вызывается в деструкторе
    inline void endGrid() noexcept;

  private:
    bool _tableCreated;
  };

  inline ImGuiPropertyGrid::ImGuiPropertyGrid(const char* gridName) :
    _tableCreated(false)
  {
    _tableCreated = ImGui::BeginTable(gridName, 2, 0);
    if(!_tableCreated) return;

    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(1);
    ImGui::PushItemWidth(-FLT_MIN);
  }

  inline ImGuiPropertyGrid::~ImGuiPropertyGrid() noexcept
  {
    endGrid();
  }

  inline void ImGuiPropertyGrid::addRow(const char* label,
                                        const char* toolTip)
  {
    if(!_tableCreated) return;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label);
    if(toolTip != nullptr) ImGui::SetItemTooltip(toolTip);
    ImGui::TableSetColumnIndex(1);
  }

  inline void ImGuiPropertyGrid::endGrid() noexcept
  {
    if(!_tableCreated) return;
    ImGui::EndTable();
    _tableCreated = false;
  }
}