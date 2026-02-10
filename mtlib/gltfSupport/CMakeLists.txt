cmake_minimum_required (VERSION 3.10)

project(gltfSupport)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${TINY_GLTF_PATH})

set(SOURCES
    "GLTFImporter.cpp")

add_library(gltfSupport
            ${SOURCES})
