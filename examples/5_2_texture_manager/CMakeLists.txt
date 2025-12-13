cmake_minimum_required (VERSION 3.10)

project (texture_manager_example)

include_directories(.
                    ../..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (texture_manager_example
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(texture_manager_example
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      ddsSupport
                      util
                      vkr
                      resourceManagement
                      technique
                      gui)
