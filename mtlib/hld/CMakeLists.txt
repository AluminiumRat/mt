cmake_minimum_required (VERSION 3.10)

project(hld)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH})

set(SOURCES
    "drawCommand/CommandMemoryPool.cpp"
    "drawCommand/DrawCommandList.cpp"
    "drawScene/Drawable.cpp"
    "meshDrawable/MeshAsset.cpp"
    "meshDrawable/MeshDrawable.cpp"
    "meshDrawable/MeshDrawCommand.cpp"
    "drawScene/Drawable3D.cpp"
    "drawScene/DrawScene.cpp"
    "FrameContext.cpp"
    "HLDLib.cpp")

add_library(hld
            ${SOURCES})
