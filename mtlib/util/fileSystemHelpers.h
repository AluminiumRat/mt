#pragma once

#include <filesystem>
#include <string>

#include <util/Assert.h>

namespace mt
{
  inline std::string pathToUtf8(const std::filesystem::path& path);
  inline std::filesystem::path utf8ToPath(const std::string& filename);

  //  Перевести pathToSave в формат, пригодный для сохранения в файле проекта
  //  Если pathToSave - это абсолютный путь и pathToSave лежит в projectFolder
  //    или её подпапке, то возвращает путь относительно projectFolder, иначе
  //    возвращает исходный pathToSave
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

    auto [mismatchInFile, mismatchInFolder] =
                                        std::mismatch(normalizedPath.begin(),
                                                      normalizedPath.end(),
                                                      projectFolderNorm.begin(),
                                                      projectFolderNorm.end());
    //  Проверяем, а лежит ли файл в указанной папке
    if(mismatchInFolder != projectFolderNorm.end()) return pathToSave;

    //  Восстанавливаем относительный путь от точки расхождения
    std::filesystem::path storedPath;
    for(std::filesystem::path::const_iterator iPath = mismatchInFile;
        iPath != normalizedPath.end();
        iPath++)
    {
      storedPath = storedPath  / *iPath;
    }
    return storedPath;
  }

  inline std::filesystem::path restoreAbsolutePath(
                                    const std::filesystem::path& storedPath,
                                    const std::filesystem::path& projectFolder)
  {
    if (storedPath.empty()) return storedPath;
    if (storedPath.is_absolute()) return storedPath;
    MT_ASSERT(projectFolder.is_absolute());
    return projectFolder / storedPath;
  }
}