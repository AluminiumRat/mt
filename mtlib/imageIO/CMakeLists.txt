cmake_minimum_required (VERSION 3.10)

project(imageIO)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${DDSIMAGE_PATH}
                    ${STB_PATH})

set(SOURCES
    "ddsSupport.cpp"
    "stbSupport.cpp")

add_library(imageIO
            ${SOURCES})
