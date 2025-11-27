cmake_minimum_required (VERSION 3.10)

project(technique)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(Shaderc REQUIRED)
find_package(YamlCPP REQUIRED)

find_path(GLFW_PATH
          include/GLFW/glfw3.h
          HINTS "$ENV{GLFW_PATH}"
          REQUIRED)

find_path(GLM_PATH
          glm/glm.hpp
          HINTS "$ENV{GLM_PATH}"
          REQUIRED)

find_path(VKMEMALLOC_PATH
          vk_mem_alloc.h
          HINTS "$ENV{VKMEMALLOC_PATH}"
          REQUIRED)

set(Spirv_reflect_INCLUDE_DIRS "../third_party/spirv_reflect")

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${Shaderc_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include
                    ${YamlCPP_INCLUDE_DIRS}
                    ${Spirv_reflect_INCLUDE_DIRS})

set(SOURCES
    "../third_party/spirv_reflect/spirv_reflect.c"
    "PassConfigurator.cpp"
    "Selection.cpp"
    "ShaderCompilator.cpp"
    "Technique.cpp"
    "TechniqueConfigurator.cpp"
    "TechniqueLoader.cpp"
    "TechniquePass.cpp"
    "TechniqueResource.cpp"
    "TechniqueUniformBlock.cpp"
    "UniformVariable.cpp"
    )

add_library(technique
            ${SOURCES})

target_compile_definitions(technique PRIVATE YAML_CPP_STATIC_DEFINE)