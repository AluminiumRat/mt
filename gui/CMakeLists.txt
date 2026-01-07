cmake_minimum_required (VERSION 3.10)

project(gui)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include
                    ${YamlCPP_INCLUDE_DIRS}
                    ${IMGUI_PATH})

set(SOURCES
    "CameraManipulator/CameraManipulator.cpp"
    "CameraManipulator/FlatCameraManipulator.cpp"
    "CameraManipulator/OrbitalCameraManipulator.cpp"
    "TextureViewer/TextureViewer.cpp"
    "TextureViewer/TextureViewerManager.cpp"
    "AsyncTaskGui.cpp"
    "BaseWindow.cpp"
    "GUILib.cpp"
    "GUIWindow.cpp"
    "modalDialogs.cpp"
    "RenderWindow.cpp"
    ${IMGUI_PATH}"/imgui.cpp"
    ${IMGUI_PATH}"/imgui_draw.cpp"
    ${IMGUI_PATH}"/imgui_tables.cpp"
    ${IMGUI_PATH}"/imgui_widgets.cpp"
    ${IMGUI_PATH}"/backends/imgui_impl_glfw.cpp"
    ${IMGUI_PATH}"/backends/imgui_impl_vulkan.cpp")

add_library(gui
            ${SOURCES})

target_compile_definitions(gui PRIVATE YAML_CPP_STATIC_DEFINE)