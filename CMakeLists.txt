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
          HINTS "3rdParty/imgui"
          REQUIRED)

find_path(VKMEMALLOC_PATH
          vk_mem_alloc.h
          HINTS "$ENV{VKMEMALLOC_PATH}"
          REQUIRED)

find_path(DDSIMAGE_PATH
          dds.hpp
          HINTS "3rdParty/dds_image/include"
          REQUIRED)

find_path(STB_PATH
          stb_image.h
          HINTS "3rdParty/stb"
          REQUIRED)

find_path(TINY_GLTF_PATH
          tiny_gltf.h
          HINTS "3rdParty/tinygltf"
          REQUIRED)

add_subdirectory(mtlib/asyncTask)
add_subdirectory(mtlib/gltfSupport)
add_subdirectory(mtlib/gui)
add_subdirectory(mtlib/hld)
add_subdirectory(mtlib/imageIO)
add_subdirectory(mtlib/resourceManagement)
add_subdirectory(mtlib/technique)
add_subdirectory(mtlib/techniquePropertySet)
add_subdirectory(mtlib/util)
add_subdirectory(mtlib/vkr)

add_subdirectory(app/examples/1_0_dumpHardware)
add_subdirectory(app/examples/1_1_raw_vkr)
add_subdirectory(app/examples/2_0_technique_manual)
add_subdirectory(app/examples/2_1_technique_load)
add_subdirectory(app/examples/2_2_technique_compute)
add_subdirectory(app/examples/3_0_imgui)
add_subdirectory(app/examples/4_0_image_load)
add_subdirectory(app/examples/5_0_file_watcher)
add_subdirectory(app/examples/5_1_async_tasks)
add_subdirectory(app/examples/5_2_resource_managers)
add_subdirectory(app/examples/6_0_hld_base)
add_subdirectory(app/examples/6_1_hld_gltfScene)

add_subdirectory(app/textureProcessor)
