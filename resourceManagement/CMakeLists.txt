cmake_minimum_required (VERSION 3.10)

project(resourceManagement)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

set(SOURCES
    "FileWatcher.cpp"
    "TextureManager.cpp")

add_library(resourceManagement
            ${SOURCES})
