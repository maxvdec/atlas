/*
 rendering.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Rendering functions and utilities
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_RENDERING_HPP
#define ATLAS_RENDERING_HPP

#include "atlas/units.hpp"
#include <string>
#include <vector>

struct CoreVertex {
    float x;
    float y;
    float z;

    Color color;
};

enum class CoreShaderType {
    Vertex,
    Fragment,
};

struct CoreShader {
    unsigned int ID;

    CoreShader(std::string code, CoreShaderType type);
};

struct CoreShaderProgram {
    unsigned int ID;

    CoreShaderProgram(const std::vector<CoreShader> &shaders);

    void use() const;
};

struct CoreVertexAttributes {
    unsigned int VBO;
    unsigned int VAO;
};

struct CoreObject {
    std::vector<CoreVertex> vertices;
    CoreVertexAttributes attributes;
    std::vector<CoreShader> shaders;
    CoreShader vertexShader;
    CoreShader fragmentShader;
    CoreShaderProgram program;

    void initialize();

    std::vector<float> makeVertexData() const;
    std::vector<CoreShader> makeShaderList() const;
};

#endif // ATLAS_RENDERING_HPP
