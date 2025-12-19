#include <gui/modalDialogs.h>
#include <util/Abort.h>

namespace fs = std::filesystem;

#ifdef WIN32

#include <windows.h>

std::wstring utf8ToUtf16(const char* utf8String)
{
  int size = MultiByteToWideChar(CP_UTF8, 0, utf8String, -1, nullptr, 0);
  if(size == 0) return L"";

  std::wstring utf16String;
  utf16String.resize(size - 1);

  MultiByteToWideChar(CP_UTF8,
                      0,
                      utf8String,
                      -1,
                      utf16String.data(),
                      size);
  return utf16String;
}

static HWND getHWND(mt::BaseWindow* window)
{
  return window == nullptr ? NULL : (HWND)window->platformDescriptor();
}

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

fs::path mt::openFileDialog(BaseWindow* ownerWindow,
                            const FileFilters& filters,
                            const fs::path& initialDir)
{
  std::vector<WCHAR> filterString = buildFiltersString(filters);

  WCHAR filenameBuffer[2048] = {0};

  OPENFILENAMEW dlgInfo{};
  dlgInfo.lStructSize = sizeof(dlgInfo);
  dlgInfo.lpstrFile = filenameBuffer;
  dlgInfo.nMaxFile = sizeof(filenameBuffer) / sizeof(filenameBuffer[0]);
  dlgInfo.hwndOwner = getHWND(ownerWindow);
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

fs::path mt::saveFileDialog(BaseWindow* ownerWindow,
                            const FileFilters& filters,
                            const fs::path& initialDir)
{
  std::vector<WCHAR> filterString = buildFiltersString(filters);

  WCHAR filenameBuffer[2048] = {0};

  OPENFILENAMEW dlgInfo{};
  dlgInfo.lStructSize = sizeof(dlgInfo);
  dlgInfo.lpstrFile = filenameBuffer;
  dlgInfo.nMaxFile = sizeof(filenameBuffer) / sizeof(filenameBuffer[0]);
  dlgInfo.hwndOwner = getHWND(ownerWindow);
  dlgInfo.lpstrInitialDir = initialDir.c_str();
  dlgInfo.lpstrFilter = filterString.data();

  //  GetSaveFileNameW меняет рабочую папку приложения, поэтому сохраним
  //  текущую, чтобы потом восстановить
  fs::path workingDirectory = fs::current_path();

  bool fileSelected = GetSaveFileNameW(&dlgInfo);

  fs::current_path(workingDirectory);

  if(fileSelected) return filenameBuffer;
  else return "";
}

bool mt::yesNoQuestionDialog( BaseWindow* ownerWindow,
                              const char* caption,
                              const char* question,
                              bool defaultValue)
{
  int answer = MessageBoxW( getHWND(ownerWindow),
                            utf8ToUtf16(question).c_str(),
                            utf8ToUtf16(caption).c_str(),
                            MB_ICONQUESTION |
                              MB_YESNO |
                              (defaultValue? MB_DEFBUTTON1 : MB_DEFBUTTON2));

  return answer == IDYES;
}

#else
fs::path mt::openFileDialog(BaseWindow* ownerWindow,
                            const FileFilters& filters,
                            const fs::path& initialDir)
{
  Abort("Not implemented");
}

fs::path mt::saveFileDialog(BaseWindow* ownerWindow,
                            const FileFilters& filters,
                            const fs::path& initialDir)
{
  Abort("Not implemented");
}

bool mt::yesNoQuestionDialog( BaseWindow* ownerWindow,
                              const char* caption,
                              const char* question,
                              bool defaultValue)
{
  Abort("Not implemented");
}

#endif