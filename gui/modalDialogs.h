#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <gui/BaseWindow.h>

namespace mt
{
  class BaseWindow;

  // Фильтр для выбора типа файла в диаоге
  struct FileFilter
  {
    std::string expression;   //  Выражение, по которому происходит фильтрование
                              //  Напроимер "*.*" или "*.txt"
    std::string description;  //  Текстовое описание для пользователя
  };
  using FileFilters = std::vector<FileFilter>;

  std::filesystem::path openFileDialog(
                                      BaseWindow* ownerWindow,
                                      const FileFilters& filters,
                                      const std::filesystem::path& initialDir);

  std::filesystem::path saveFileDialog(
                                      BaseWindow* ownerWindow,
                                      const FileFilters& filters,
                                      const std::filesystem::path& initialDir);

  //  Диалог с вопросом, на который пользователь может ответить только
  //    "Да" и "Нет"
  //  Параметр question в UTF8
  //  defaultValue задает, на какой кнопке будет фокус
  //  Возвращает ответ пользователя: true - "Да", false - "Нет"
  bool yesNoQuestionDialog( BaseWindow* ownerWindow,
                            const char* caption,
                            const char* question,
                            bool defaultValue);
}