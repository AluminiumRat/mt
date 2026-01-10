cmake_minimum_required (VERSION 3.10)

project (dumpHardware)

include_directories(../../../mtlib
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

add_executable (dumpHardware
                "main.cpp")

target_link_libraries(dumpHardware
                      ${Vulkan_LIBRARIES}
                      util
                      vkr)
