cmake_minimum_required (VERSION 3.10)

project (hld_Test)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (hld_Test
                "main.cpp"
                "TestDrawStage.cpp"
                "TestWindow.cpp")

target_link_libraries(hld_Test
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      ddsSupport
                      util
                      vkr
                      technique
                      gui
                      hld)
