#pragma once

#include <filesystem>

#include <imgui.h>

#include <util/Bimap.h>
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

  //  Виджет - комбобокс для выбора значения enum-а
  //  map - бимапа для конвертации значения в строку и обратно
  //  label - лэйбл и imgui ID для контрола
  template<typename EnumType>
  inline EnumType enumSelectionCombo( const char* label,
                                      EnumType currentValue,
                                      const Bimap<EnumType>& map);

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

  template<typename EnumType>
  inline EnumType enumSelectionCombo<EnumType>( const char* label,
                                                EnumType currentValue,
                                                const Bimap<EnumType>& map)
  {
    //  Ищем надпись для превью
    const std::string& previewText = map[currentValue];
    if (ImGui::BeginCombo(label, previewText.c_str(), 0))
    {
      for(typename Bimap<EnumType>::const_iterator iValue = map.begin();
          iValue != map.end();
          iValue++)
      {
        if(ImGui::Selectable( iValue->first.c_str(),
                              currentValue == iValue->second))
        {
          currentValue = iValue->second;
        }
      }
      ImGui::EndCombo();
    }
    return currentValue;
  }
}