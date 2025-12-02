cmake_minimum_required (VERSION 3.10)

project (rawVkr)

include_directories(.
                    ../..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (rawVkr
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(rawVkr
                      ${Vulkan_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      util
                      vkr
                      gui)
