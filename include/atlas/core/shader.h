/*
 shader.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader definition and structure
 Copyright (c) 2025 maxvdec
*/

#ifndef SHADER_H
#define SHADER_H

#include "atlas/units.h"
#include <optional>

#define DEFAULT_FRAG_SHADER Debug
#define DEFAULT_VERT_SHADER Debug

enum class AtlasVertexShader {
    Debug,
};

struct VertexShader {
    const char *source;

    static VertexShader fromDefaultShader(AtlasVertexShader shader);
    static VertexShader fromSource(const char *source);
    void compile();

    Id shaderId;
};

enum class AtlasFragmentShader {
    Debug,
};

struct FragmentShader {
    const char *source;

    static FragmentShader fromDefaultShader(AtlasFragmentShader shader);
    static FragmentShader fromSource(const char *source);
    void compile();

    Id shaderId;
};

struct ShaderProgram {
    VertexShader vertexShader;
    FragmentShader fragmentShader;

    void compile();

    static ShaderProgram defaultProgram();

    Id programId;
};

#endif // SHADER_H
