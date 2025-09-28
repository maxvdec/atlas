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

#include <glm/glm.hpp>

#define DEFAULT_FRAG_SHADER Main
#define DEFAULT_VERT_SHADER Main

enum class AtlasVertexShader {
    Debug,
    Color,
    Main,
    Texture,
    Fullscreen,
    Skybox,
    Depth,
    Particle
};

enum class ShaderCapability { Lighting, Textures, Shadows };

struct VertexShader {
    const char *source;

    static VertexShader fromDefaultShader(AtlasVertexShader shader);
    static VertexShader fromSource(const char *source);
    void compile();

    std::vector<uint32_t> desiredAttributes;
    std::vector<ShaderCapability> capabilities;

    Id shaderId;
};

enum class AtlasFragmentShader {
    Debug,
    Color,
    Main,
    Texture,
    Fullscreen,
    Skybox,
    Empty,
    Particle
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
    static ShaderProgram fromDefaultShaders(AtlasVertexShader vShader,
                                            AtlasFragmentShader fShader);

    Id programId;

    std::vector<uint32_t> desiredAttributes;

    std::vector<ShaderCapability> capabilities;

    void setUniform4f(std::string name, float v0, float v1, float v2, float v3);
    void setUniform3f(std::string name, float v0, float v1, float v2);
    void setUniform2f(std::string name, float v0, float v1);
    void setUniform1f(std::string name, float v0);
    void setUniform1i(std::string name, int v0);
    void setUniformMat4f(std::string name, const glm::mat4 &matrix);
    void setUniformBool(std::string name, bool value);
};

#endif // SHADER_H
