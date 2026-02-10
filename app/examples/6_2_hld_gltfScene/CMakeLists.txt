cmake_minimum_required (VERSION 3.10)

project (hld_gltfScene)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH})

add_executable (hld_gltfScene
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(hld_gltfScene
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      glbSupport
                      gui
                      hld
                      imageIO
                      resourceManagement
                      technique
                      util
                      vkr)
