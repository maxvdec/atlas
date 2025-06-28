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
#include <functional>
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
    int id = 0;
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

using RenderingFn = std::function<void(CoreObject *)>;

struct Renderer {
    std::vector<RenderingFn> dispactchers;
    std::vector<CoreObject *> registeredObjects;

    static Renderer &instance() {
        static Renderer inst;
        return inst;
    }

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void registerObject(CoreObject *object, RenderingFn dispatcher);

    void dispatchAll();

  private:
    Renderer() = default;
};

#endif // ATLAS_RENDERING_HPP
