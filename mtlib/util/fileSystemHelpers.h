#pragma once

#include <filesystem>
#include <string>

#include <util/Assert.h>

namespace mt
{
  inline std::string pathToUtf8(const std::filesystem::path& path);
  inline std::filesystem::path utf8ToPath(const std::string& filename);

  //  Перевести pathToSave в формат, пригодный для сохранения в файле проекта
  //  Если pathToSave - это абсолютный путь и существует относительный
  //    путь из projectFolder в pathToSave, то возвращает путь относительно
  //    projectFolder, иначе возвращает исходный pathToSave
  inline std::filesystem::path makeStoredPath(
                                    const std::filesystem::path& pathToSave,
                                    const std::filesystem::path& projectFolder);

  //  Восстановить путь для файла из сохраненного в файле проекта
  //  Если storedPath - относительный, то возвращает абсолютный путь,
  //    построенный из projectFolder + storedPath
  inline std::filesystem::path restoreAbsolutePath(
                                    const std::filesystem::path& storedPath,
                                    const std::filesystem::path& projectFolder);

  //-------------------------------------------------------------------------
  //  Реализации функций
  inline std::string pathToUtf8(const std::filesystem::path& path)
  {
    return (const char*)path.u8string().c_str();
  }

  inline std::filesystem::path utf8ToPath(const std::string& filename)
  {
    return std::filesystem::path((char8_t*)filename.c_str());
  }

  inline std::filesystem::path makeStoredPath(
                                    const std::filesystem::path& pathToSave,
                                    const std::filesystem::path& projectFolder)
  {
    if(pathToSave.empty()) return pathToSave;
    if (!pathToSave.is_absolute()) return pathToSave;
    MT_ASSERT(projectFolder.is_absolute());

    std::filesystem::path normalizedPath = pathToSave.lexically_normal();
    std::filesystem::path projectFolderNorm = projectFolder.lexically_normal();

    std::filesystem::path storedPath =
                          normalizedPath.lexically_relative(projectFolderNorm);
    if(storedPath.empty()) return pathToSave;
    return storedPath;
  }

  inline std::filesystem::path restoreAbsolutePath(
                                    const std::filesystem::path& storedPath,
                                    const std::filesystem::path& projectFolder)
  {
    if (storedPath.empty()) return storedPath;
    if (storedPath.is_absolute()) return storedPath;
    MT_ASSERT(projectFolder.is_absolute());
    return (projectFolder / storedPath).lexically_normal();
  }
}