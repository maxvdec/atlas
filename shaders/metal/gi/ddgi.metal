/*
 ddgi.metal
 As part of the Atlas project
 Created by Max Van den Eynde in 2026
 --------------------------------------------------
 Description: Shader for the DDGI implementation in Atlas.
 Copyright (c) 2026 Max Van den Eynde
*/

#include <metal_stdlib>
using namespace metal;

kernel void main0(texture2d<half, access::write> outTexture [[texture(0)]],
                  uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height()) {
        return;
    }
    outTexture.write(half4(1.0h, 0.0h, 0.0h, 1.0h), gid);
}
