#include <Project.h>

Project::Project(const std::filesystem::path& file) :
  _projectFile(file)
{
}

void Project::save(const std::filesystem::path& file)
{
  _projectFile = file;
}
