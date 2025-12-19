cmake_minimum_required (VERSION 3.10)

project (textureProcessor)

include_directories(.
                    ..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (textureProcessor
                "Application.cpp"
                "main.cpp"
                "MainWindow.cpp")

target_link_libraries(textureProcessor
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
