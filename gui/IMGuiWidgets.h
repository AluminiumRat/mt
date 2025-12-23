#pragma once

#include <filesystem>

#include <imgui.h>

#include <util/fileSystemHelpers.h>

namespace mt
{
  //  Виджет - строка для выбора файла в проперти гриде
  //  controlId - уникальное имя для контрола, будет использоваться как ID для
  //    ImGUI
  //  currentFilePath - текущий выбранный файл
  //  Возвращает true, если нажата кнока и надо выбрать новый файл, false
  //    в противном случае
  inline bool fileSelectionLine(
                        const char* controlId,
                        const std::filesystem::path& currentFilePath) noexcept;

  //-------------------------------------------------------------------------
  //  Реализации функций
  inline bool fileSelectionLine(const char* controlId,
                                const std::filesystem::path& filePath) noexcept
  {
    ImGui::PushID(controlId);
    bool pressed =  ImGui::SmallButton("...");
    ImGui::SameLine();
    ImGui::Text(mt::pathToUtf8(filePath.filename()).c_str());
    ImGui::SetItemTooltip(mt::pathToUtf8(filePath).c_str());
    ImGui::PopID();
    return pressed;
  }
}