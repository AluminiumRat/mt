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
  //  value - значение, которое котролируется виджетом
  //  map - бимапа для конвертации значения в строку и обратно
  //  label - лэйбл и imgui ID для контрола
  //  Возвращает true, если значение value было изменено, иначе false
  template<typename EnumType>
  inline bool enumSelectionCombo( const char* label,
                                  EnumType& value,
                                  const Bimap<EnumType>& map);

  //-------------------------------------------------------------------------
  //  Реализации функций
  inline bool fileSelectionLine(const char* controlId,
                                const std::filesystem::path& filePath) noexcept
  {
    ImGui::PushID(controlId);
    bool pressed =  ImGui::SmallButton("...");
    ImGui::SameLine();
    if(!filePath.empty())
    {
      ImGui::Text(mt::pathToUtf8(filePath.filename()).c_str());
      ImGui::SetItemTooltip(mt::pathToUtf8(filePath).c_str());
    }
    else
    {
      ImGui::Text("none");
      ImGui::SetItemTooltip("Click ... to select a file");
    }
    ImGui::PopID();
    return pressed;
  }

  template<typename EnumType>
  inline bool enumSelectionCombo<EnumType>( const char* label,
                                            EnumType& value,
                                            const Bimap<EnumType>& map)
  {
    const std::string& previewText = map[value];

    EnumType newValue = value;
    if (ImGui::BeginCombo(label, previewText.c_str(), 0))
    {
      for(typename Bimap<EnumType>::const_iterator iValue = map.begin();
          iValue != map.end();
          iValue++)
      {
        if(ImGui::Selectable( iValue->first.c_str(),
                              value == iValue->second))
        {
          newValue = iValue->second;
        }
      }
      ImGui::EndCombo();
    }

    if(value != newValue)
    {
      value = newValue;
      return true;
    }
    return false;
  }
}