cmake_minimum_required (VERSION 3.10)

project (hld_opaque)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (hld_opaque
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(hld_opaque
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      ddsSupport
                      gui
                      util
                      hld
                      resourceManagement
                      technique
                      vkr)
