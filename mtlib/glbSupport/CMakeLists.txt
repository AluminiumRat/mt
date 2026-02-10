cmake_minimum_required (VERSION 3.10)

project(glbSupport)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${TINY_GLTF_PATH})

set(SOURCES
    "glbSupport.cpp")

add_library(glbSupport
            ${SOURCES})
