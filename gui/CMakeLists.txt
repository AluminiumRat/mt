cmake_minimum_required (VERSION 3.10)

project(gui)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include
                    ${IMGUI_PATH})

set(SOURCES
    "BaseWindow.cpp"
    "GUILib.cpp"
    "GUIWindow.cpp"
    "RenderWindow.cpp"
    ${IMGUI_PATH}"/imgui.cpp"
    ${IMGUI_PATH}"/imgui_draw.cpp"
    ${IMGUI_PATH}"/imgui_tables.cpp"
    ${IMGUI_PATH}"/imgui_widgets.cpp"
    ${IMGUI_PATH}"/backends/imgui_impl_glfw.cpp"
    ${IMGUI_PATH}"/backends/imgui_impl_vulkan.cpp")

add_library(gui
            ${SOURCES})
