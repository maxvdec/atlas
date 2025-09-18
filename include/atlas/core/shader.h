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
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#ifdef ATLAS_LIBRARY_IMPL
#include <glm/glm.hpp>
#endif

#define DEFAULT_FRAG_SHADER Color
#define DEFAULT_VERT_SHADER Color

enum class AtlasVertexShader {
    Debug,
    Color,
};

struct VertexShader {
    const char *source;

    static VertexShader fromDefaultShader(AtlasVertexShader shader);
    static VertexShader fromSource(const char *source);
    void compile();

    std::vector<uint32_t> desiredAttributes;

    Id shaderId;
};

enum class AtlasFragmentShader {
    Debug,
    Color,
};

struct FragmentShader {
    const char *source;

    static FragmentShader fromDefaultShader(AtlasFragmentShader shader);
    static FragmentShader fromSource(const char *source);
    void compile();

    Id shaderId;
};

struct LayoutDescriptor {
    int layoutPos;
    int size;
    unsigned int type;
    bool normalized;
    int stride;
    std::size_t offset;
};

struct ShaderProgram {
    VertexShader vertexShader;
    FragmentShader fragmentShader;

    void compile();

    static ShaderProgram defaultProgram();

    Id programId;

    std::vector<uint32_t> desiredAttributes;

#ifdef ATLAS_LIBRARY_IMPL
    void setUniform4f(std::string name, float v0, float v1, float v2, float v3);
    void setUniform3f(std::string name, float v0, float v1, float v2);
    void setUniform2f(std::string name, float v0, float v1);
    void setUniform1f(std::string name, float v0);
    void setUniform1i(std::string name, int v0);
    void setUniformMat4f(std::string name, const glm::mat4 &matrix);
#endif
};

#endif // SHADER_H
