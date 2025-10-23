cmake_minimum_required (VERSION 3.10)

project (vkTest)

set(CMAKE_MODULE_PATH &{CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(Shaderc REQUIRED)

find_path(GLFW_PATH
          include/GLFW/glfw3.h
          HINTS "$ENV{GLFW_PATH}"
          REQUIRED)

find_path(GLM_PATH
          glm/glm.hpp
          HINTS "$ENV{GLM_PATH}"
          REQUIRED)

find_path(VKMEMALLOC_PATH
          vk_mem_alloc.h
          HINTS "$ENV{VKMEMALLOC_PATH}"
          REQUIRED)

include_directories(.
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${Shaderc_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include)

add_executable (vkTest
                "util/Log.cpp"
                "vkr/queue/CommandBuffer.cpp"
                "vkr/queue/CommandPool.cpp"
                "vkr/queue/CommandPoolSet.cpp"
                "vkr/queue/CommandProducer.cpp"
                "vkr/queue/CommandQueue.cpp"
                "vkr/queue/Fence.cpp"
                "vkr/queue/QueueFamiliesInfo.cpp"
                "vkr/queue/QueueSources.cpp"
                "vkr/queue/Semaphore.cpp"
                "vkr/queue/TimelineSemaphore.cpp"
                "vkr/queue/UniformMemoryPool.cpp"
                "vkr/queue/VolatileDescriptorPool.cpp"
                "vkr/DescriptorCounter.cpp"
                "vkr/DescriptorPool.cpp"
                "vkr/Device.cpp"
                "vkr/Image.cpp"
                "vkr/ImageSlice.cpp"
                "vkr/PhysicalDevice.cpp"
                "vkr/PlainBuffer.cpp"
                "vkr/SwapChain.cpp"
                "vkr/VKRLib.cpp"
                "vkr/WindowSurface.cpp"
                "vkr/Win32WindowSurface.cpp"
                "main.cpp")

target_link_libraries(vkTest
                      ${Vulkan_LIBRARIES}
                      ${Shaderc_LIBRARIES}
                      ${GLFW_PATH}/lib-vc2022/glfw3.lib)
