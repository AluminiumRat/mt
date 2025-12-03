cmake_minimum_required (VERSION 3.10)

project(ddsSupport)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${DDSIMAGE_PATH})

set(SOURCES
    "ddsSupport.cpp")

add_library(ddsSupport
            ${SOURCES})
