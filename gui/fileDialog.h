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
}