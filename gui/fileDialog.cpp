#include <gui/fileDialog.h>
#include <util/Abort.h>

namespace fs = std::filesystem;

#ifdef WIN32

#include <codecvt>

#include <windows.h>

//  Из списка UTF-8 строк фильтров сформировать UTF-16 единую строку-фильтр
//    Используется как аргумент для вызова файловых диалогов
//  Используем std::vector<WCHAR> вместо wstring, так как результирующая строка
//    может содержать нулевые символы в середине
//  Смотри https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
static std::vector<WCHAR> buildFiltersString(const mt::FileFilters& filters)
{
  std::vector<WCHAR> filtersString;
  for(const mt::FileFilter& filter : filters)
  {
    WCHAR buffer[1024];

    // Переводим описание в UTF8
    int processed = MultiByteToWideChar(CP_UTF8,
                                        0,
                                        filter.description.c_str(),
                                        (int)filter.description.size(),
                                        buffer,
                                        sizeof(buffer) / sizeof(buffer[0]));
    if(processed > 0)
    {
      filtersString.insert( filtersString.end(),
                            buffer,
                            buffer + processed);
      // Разделитель
      filtersString.push_back(0);
    }
    else continue;

    // Переводим сам фильтр в utf8
    processed = MultiByteToWideChar(CP_UTF8,
                                    0,
                                    filter.expression.c_str(),
                                    (int)filter.expression.size(),
                                    buffer,
                                    sizeof(buffer) / sizeof(buffer[0]));
    if(processed > 0)
    {
      filtersString.insert( filtersString.end(),
                            buffer,
                            buffer + processed);
      // Разделитель между отдельными фильрами
      filtersString.push_back(0);
    }
  }

  // Двойной ноль - признак конца строки-фильтра
  filtersString.push_back(0);
  filtersString.push_back(0);

  return filtersString;
}

std::filesystem::path mt::openFileDialog( BaseWindow* ownerWindow,
                                          const FileFilters& filters,
                                          const fs::path& initialDir)
{
  std::vector<WCHAR> filterString = buildFiltersString(filters);

  WCHAR filenameBuffer[2048] = {0};

  OPENFILENAMEW dlgInfo{};
  dlgInfo.lStructSize = sizeof(dlgInfo);
  dlgInfo.lpstrFile = filenameBuffer;
  dlgInfo.nMaxFile = sizeof(filenameBuffer) / sizeof(filenameBuffer[0]);
  dlgInfo.hwndOwner = ownerWindow == nullptr ?
                        NULL :
                        (HWND)ownerWindow->platformDescriptor();
  dlgInfo.lpstrInitialDir = initialDir.c_str();
  dlgInfo.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  dlgInfo.lpstrFilter = filterString.data();

  //  GetOpenFileNameW меняет рабочую папку приложения, поэтому сохраним
  //  текущую, чтобы потом восстановить
  fs::path workingDirectory = fs::current_path();

  bool fileSelected = GetOpenFileNameW(&dlgInfo);

  fs::current_path(workingDirectory);

  if(fileSelected) return filenameBuffer;
  else return "";
}

#else
std::filesystem::path mt::openFileDialog() noexcept
{
  Abort("Not implemented");
}
#endif