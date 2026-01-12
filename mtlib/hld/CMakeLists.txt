cmake_minimum_required (VERSION 3.10)

project(hld)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

set(SOURCES
    "DrawStage.cpp"
    "FrameContext.cpp"
    "HLDLib.cpp"
    )

add_library(hld
            ${SOURCES})
