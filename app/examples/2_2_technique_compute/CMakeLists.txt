cmake_minimum_required (VERSION 3.10)

project (technique_compute)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (technique_compute
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(technique_compute
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      util
                      vkr
                      technique
                      gui)
