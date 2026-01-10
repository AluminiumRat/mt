#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <gui/BaseWindow.h>

namespace mt
{
  class BaseWindow;

  //  Сообщение об ошибке
  void errorDialog( const BaseWindow* ownerWindow,
                    const char* caption,
                    const char* message) noexcept;

  //  Диалог с вопросом, на который пользователь может ответить только
  //    "Да" и "Нет"
  //  Параметр question в UTF8
  //  defaultValue задает, на какой кнопке будет фокус
  //  Возвращает ответ пользователя: true - "Да", false - "Нет"
  bool yesNoQuestionDialog( const BaseWindow* ownerWindow,
                            const char* caption,
                            const char* question,
                            bool defaultValue)  noexcept;

  //  Диалог с вопросом, на который пользователь может ответить только
  //    "Да" ,"Нет" или "отменить"
  //  Параметр question в UTF8
  enum QuestionButton
  {
    YES_BUTTON,
    NO_BUTTON,
    CANCEL_BUTTON
  };
  QuestionButton yesNoCancelDialog( const BaseWindow* ownerWindow,
                                    const char* caption,
                                    const char* question)  noexcept;

  // Фильтр для выбора типа файла в диаоге
  struct FileFilter
  {
    std::string expression;   //  Выражение, по которому происходит фильтрование
                              //  Напроимер "*.*" или "*.txt"
    std::string description;  //  Текстовое описание для пользователя
  };
  using FileFilters = std::vector<FileFilter>;

  //  Диалог "Открыть файл"
  //  Если возвращает пустой path, значит пользователь нажал отмену, либо
  //    произошла ошибка
  std::filesystem::path openFileDialog(
                              const BaseWindow* ownerWindow,
                              const FileFilters& filters,
                              const std::filesystem::path& initialDir) noexcept;

  //  Диалог "Сохранить файл"
  //  Если возвращает пустой path, значит пользователь нажал отмену, либо
  //    произошла ошибка
  std::filesystem::path saveFileDialog(
                              const BaseWindow* ownerWindow,
                              const FileFilters& filters,
                              const std::filesystem::path& initialDir) noexcept;
}