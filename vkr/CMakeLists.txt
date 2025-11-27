cmake_minimum_required (VERSION 3.10)

project(vkr)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

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

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${GLFW_PATH}/include)

set(SOURCES
    "image/Image.cpp"
    "image/ImageAccess.cpp"
    "image/ImageAccessMultiset.cpp"
    "image/ImageAccessWatcher.cpp"
    "image/ImageFormatFeatures.cpp"
    "image/ImagesAccessSet.cpp"
    "image/ImageSlice.cpp"
    "image/ImageView.cpp"
    "pipeline/AbstractPipeline.cpp"
    "pipeline/DescriptorCounter.cpp"
    "pipeline/DescriptorPool.cpp"
    "pipeline/DescriptorSet.cpp"
    "pipeline/DescriptorSetLayout.cpp"
    "pipeline/GraphicPipeline.cpp"
    "pipeline/PipelineLayout.cpp"
    "pipeline/ShaderLoader.cpp"
    "pipeline/ShaderModule.cpp"
    "queue/CommandBuffer.cpp"
    "queue/CommandPool.cpp"
    "queue/CommandPoolSet.cpp"
    "queue/CommandProducer.cpp"
    "queue/CommandProducerCompute.cpp"
    "queue/CommandProducerGraphic.cpp"
    "queue/CommandProducerTransfer.cpp"
    "queue/CommandQueue.cpp"
    "queue/CommandQueueCompute.cpp"
    "queue/CommandQueueGraphic.cpp"
    "queue/CommandQueueTransfer.cpp"
    "queue/Fence.cpp"
    "queue/QueueFamiliesInfo.cpp"
    "queue/QueueSources.cpp"
    "Sampler.cpp"
    "queue/Semaphore.cpp"
    "queue/TimelineSemaphore.cpp"
    "queue/UniformMemoryPool.cpp"
    "queue/VolatileDescriptorPool.cpp"
    "DataBuffer.cpp"
    "Device.cpp"
    "FrameBuffer.cpp"
    "FrameBufferFormat.cpp"
    "GLFWRenderWindow.cpp"
    "PhysicalDevice.cpp"
    "SwapChain.cpp"
    "VKRLib.cpp"
    "WindowSurface.cpp"
    "Win32WindowSurface.cpp"
    )

add_library(vkr
            ${SOURCES})
