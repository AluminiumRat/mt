cmake_minimum_required (VERSION 3.10)

project(techniquePropertySet)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${YamlCPP_INCLUDE_DIRS}
                    ${IMGUI_PATH})

set(SOURCES
    "gui.cpp"
    "TechniqueProperty.cpp"
    "TechniquePropertySet.cpp"
    )

add_library(techniquePropertySet
            ${SOURCES})

target_compile_definitions(techniquePropertySet PRIVATE YAML_CPP_STATIC_DEFINE)