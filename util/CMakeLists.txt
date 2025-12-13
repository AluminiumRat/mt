cmake_minimum_required (VERSION 3.10)

project(util)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    )

set(SOURCES
    "ContentLoader.cpp"
    "Log.cpp"
    "vkMeta.cpp"
    )

add_library(util
            ${SOURCES})
