cmake_minimum_required (VERSION 3.10)

project (file_watcher)

include_directories(.
                    ../..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (file_watcher
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(file_watcher
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      util
                      vkr
                      gui
                      resourceManagement)
