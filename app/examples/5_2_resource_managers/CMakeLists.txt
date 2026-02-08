cmake_minimum_required (VERSION 3.10)

project (resource_managers_example)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (resource_managers_example
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(resource_managers_example
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      imageIO
                      util
                      vkr
                      resourceManagement
                      technique
                      gui)
