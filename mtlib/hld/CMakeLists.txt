cmake_minimum_required (VERSION 3.10)

project(hld)

include_directories(..
                    ${GLM_PATH}
                    ${Vulkan_INCLUDE_DIRS}
                    ${VKMEMALLOC_PATH}
                    ${IMGUI_PATH}
                    ${YamlCPP_INCLUDE_DIRS})

set(SOURCES
    "colorFrameBuilder/AvgLum.cpp"
    "colorFrameBuilder/BackgroundRender.cpp"
    "colorFrameBuilder/Bloom.cpp"
    "colorFrameBuilder/ColorFrameBuilder.cpp"
    "colorFrameBuilder/ColorFrameCommonSet.cpp"
    "colorFrameBuilder/ColorFrameCommonSetBindings.cpp"
    "colorFrameBuilder/EnvironmentScene.cpp"
    "colorFrameBuilder/OpaqueColorStage.cpp"
    "colorFrameBuilder/OpaquePrepassStage.cpp"
    "colorFrameBuilder/Posteffects.cpp"
    "colorFrameBuilder/ShadowsStage.cpp"
    "colorFrameBuilder/VelocityBufferUpdater.cpp"
    "drawCommand/CommandMemoryPool.cpp"
    "drawCommand/DrawCommandList.cpp"
    "drawScene/Drawable.cpp"
    "meshDrawable/MeshAsset.cpp"
    "meshDrawable/MeshDrawable.cpp"
    "meshDrawable/MeshDrawCommand.cpp"
    "drawScene/Drawable3D.cpp"
    "drawScene/DrawScene.cpp"
    "HLDLib.cpp"
    "RegularDrawStage.cpp")

add_library(hld
            ${SOURCES})

target_compile_definitions(hld PRIVATE YAML_CPP_STATIC_DEFINE)