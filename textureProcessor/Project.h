#pragma once

#include <filesystem>

class Project
{
public:
  Project(const std::filesystem::path& file);
  Project(const Project&) = delete;
  Project& operator = (const Project&) = delete;
  ~Project() noexcept = default;

  inline const std::filesystem::path& projectFile() const;

  void save(const std::filesystem::path& file);

private:
  std::filesystem::path _projectFile;
};

inline const std::filesystem::path& Project::projectFile() const
{
  return _projectFile;
}
