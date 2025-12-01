cmake_minimum_required (VERSION 3.10)

project (imgui)

include_directories(.
                    ../..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include
                    ${IMGUI_PATH})

add_executable (imgui
                "main.cpp"
                "TestWindow.cpp"
                ${IMGUI_PATH}"/imgui.cpp"
                ${IMGUI_PATH}"/imgui_draw.cpp"
                ${IMGUI_PATH}"/imgui_tables.cpp"
                ${IMGUI_PATH}"/imgui_widgets.cpp"
                ${IMGUI_PATH}"/backends/imgui_impl_glfw.cpp"
                ${IMGUI_PATH}"/backends/imgui_impl_vulkan.cpp")

target_link_libraries(imgui
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      util
                      vkr
                      technique
                      gui)
