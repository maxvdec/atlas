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

kernel void main0(texture2d<float, access::write> outTexture [[texture(0)]], uint2 gid [[[[thread_position_in_grid]]) {
    float4 color = float4(1.0, 0.0, 0.0, 1.0);
    outTexture.write(color, gid);
}
