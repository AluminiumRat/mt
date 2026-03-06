cmake_minimum_required (VERSION 3.10)

project (rayTracing_example)

include_directories(.
                    ../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${TINY_GLTF_PATH})

add_executable (rayTracing_example
                "BLASAsset.cpp"
                "BLASImporter.cpp"
                "main.cpp"
                "TestWindow.cpp")

target_link_libraries(rayTracing_example
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
