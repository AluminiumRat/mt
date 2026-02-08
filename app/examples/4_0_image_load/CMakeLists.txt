cmake_minimum_required (VERSION 3.10)

project (image_load)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (image_load
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(image_load
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      imageIO
                      util
                      vkr
                      technique
                      gui)
