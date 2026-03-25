#include "lib/commonSet.inl"

layout( rgba16f,
        set = STATIC,
        binding = 0) uniform image2D outReprojectionBuffer;

layout( r16f,
        set = STATIC,
        binding = 1) uniform image2D depthHistory;
