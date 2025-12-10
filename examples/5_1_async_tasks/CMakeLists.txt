cmake_minimum_required (VERSION 3.10)

project (asyncTasks)

include_directories(.
                    ../..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (asyncTasks
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(asyncTasks
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      util
                      vkr
                      gui)
