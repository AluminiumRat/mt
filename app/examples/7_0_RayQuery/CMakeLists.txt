cmake_minimum_required (VERSION 3.10)

project (rayQuery)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${TINY_GLTF_PATH})

add_executable (rayQuery
                "BLASImporter.cpp"
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(rayQuery
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib
                      ${YamlCPP_LIBRARIES}
                      asyncTask
                      gltfSupport
                      gui
                      imageIO
                      resourceManagement
                      technique
                      util
                      vkr)
