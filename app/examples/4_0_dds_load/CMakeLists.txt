cmake_minimum_required (VERSION 3.10)

project (dds_load)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (dds_load
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(dds_load
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      ddsSupport
                      util
                      vkr
                      technique
                      gui)
