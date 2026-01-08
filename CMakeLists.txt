cmake_minimum_required (VERSION 3.10)

project(mt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH &{CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)
find_package(Shaderc REQUIRED)
find_package(YamlCPP REQUIRED)

find_path(GLM_PATH
          glm/glm.hpp
          HINTS "$ENV{GLM_PATH}"
          REQUIRED)

find_path(GLFW_PATH
          include/GLFW/glfw3.h
          HINTS "$ENV{GLFW_PATH}"
          REQUIRED)

find_path(IMGUI_PATH
          imgui.h
          HINTS "third_party/imgui"
          REQUIRED)

find_path(VKMEMALLOC_PATH
          vk_mem_alloc.h
          HINTS "$ENV{VKMEMALLOC_PATH}"
          REQUIRED)

find_path(DDSIMAGE_PATH
          dds.hpp
          HINTS "third_party/dds_image/include"
          REQUIRED)

add_subdirectory(asyncTask)
add_subdirectory(ddsSupport)
add_subdirectory(gui)
add_subdirectory(resourceManagement)
add_subdirectory(technique)
add_subdirectory(techniquePropertySet)
add_subdirectory(util)
add_subdirectory(vkr)

add_subdirectory(examples/1_0_dumpHardware)
add_subdirectory(examples/1_1_raw_vkr)
add_subdirectory(examples/2_0_technique_manual)
add_subdirectory(examples/2_1_technique_load)
add_subdirectory(examples/3_0_imgui)
add_subdirectory(examples/4_0_dds_load)
add_subdirectory(examples/5_0_file_watcher)
add_subdirectory(examples/5_1_async_tasks)
add_subdirectory(examples/5_2_resource_managers)

add_subdirectory(textureProcessor)
