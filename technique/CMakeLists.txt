cmake_minimum_required (VERSION 3.10)

project(technique)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${Shaderc_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${YamlCPP_INCLUDE_DIRS}
                    "../third_party/spirv_reflect")

set(SOURCES
    "../third_party/spirv_reflect/spirv_reflect.c"
    "PassConfigurator.cpp"
    "ResourceBinding.cpp"
    "Selection.cpp"
    "ShaderCompilator.cpp"
    "Technique.cpp"
    "TechniqueConfigurator.cpp"
    "TechniqueLoader.cpp"
    "TechniquePass.cpp"
    "TechniqueUniformBlock.cpp"
    "UniformVariable.cpp"
    )

add_library(technique
            ${SOURCES})

target_compile_definitions(technique PRIVATE YAML_CPP_STATIC_DEFINE)