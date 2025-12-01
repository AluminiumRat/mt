cmake_minimum_required (VERSION 3.10)

project(gui)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include)

set(SOURCES
    "RenderWindow.cpp")

add_library(gui
            ${SOURCES})
