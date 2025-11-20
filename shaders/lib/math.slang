//
// math.slang
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Math utility functions
// Copyright (c) 2025 Max Van den Eynde
//

module math;

func minor3x3(m: float4x4, row: int, col: int)->float3x3 {
    float3x3 result;
    int r = 0;
    for (int i = 0; i < 4; i++) {
        if (i == row)
            continue;
        int c = 0;
        for (int j = 0; j < 4; j++) {
            if (j == col)
                continue;
            result[c][r] = m[j][i];
            c++;
        }
        r++;
    }
    return result;
}

export func adjoint(original: float4x4)->float4x4 {
    float4x4 adj;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float3x3 minor = minor3x3(original, i, j);
            float sign = ((i + j) % 2 == 0) ? 1.0 : -1.0;
            adj[j][i] = sign * determinant(minor);
        }
    }

    return adj;
}

export func inverse(original: float4x4)->float4x4 {
    float det = determinant(original);

    if (abs(det) < 1e-6) {
        return float4x4(1.0);
    }

    return adjoint(original) / det;
}

export func computeNormalMatrix(modelMatrix: float4x4)->float3x3 {
    return transpose((float3x3)inverse(modelMatrix));
}

export func computeTBN(inout T: float3, inout B: float3, N: float3)->float3x3 {
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
    return float3x3(T, B, N);
}
