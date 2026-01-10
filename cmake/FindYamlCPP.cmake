find_path(YamlCPP_INCLUDE_DIRS
          yaml-cpp/yaml.h
          HINTS "3rdParty/yaml-cpp/include"
          REQUIRED)

if(YamlCPP_INCLUDE_DIRS)
  find_library( YamlCPP_LIBRARY_DEBUG
                NAMES yaml-cppd
                HINTS "3rdParty/yaml-cpp/build"
                REQUIRED)

  find_library( YamlCPP_LIBRARY_RELEASE
                NAMES yaml-cpp
                HINTS "3rdParty/yaml-cpp/build"
                REQUIRED)

  mark_as_advanced( YamlCPP_INCLUDE_DIRS
                    YamlCPP_LIBRARY_DEBUG
                    YamlCPP_LIBRARY_RELEASE)

  include(SelectLibraryConfigurations)
  select_library_configurations(YamlCPP)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YamlCPP DEFAULT_MSG YamlCPP_LIBRARIES YamlCPP_INCLUDE_DIRS)
