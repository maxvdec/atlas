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

#include "atlas/material.hpp"
#include "atlas/texture.hpp"
#include "atlas/units.hpp"
#include <algorithm>
#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

struct CoreVertex {
    float x;
    float y;
    float z;

    Color color;

    Size2d textCoords = Size2d(0.0f, 0.0f);

    Size3d normal = Size3d(0.0f, 0.0f, 0.0f);
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

    void setFloat(const std::string &name, float val) const;
    void setInt(const std::string &name, int val) const;
    void setBool(const std::string &name, bool value) const;
    void setMatrix4(const std::string &name, const glm::mat4 &matrix) const;
    void setVec2(const std::string &name, const glm::vec2 &vector) const;
    void setVec3(const std::string &name, const glm::vec3 &vector) const;

    inline bool symbolExists(const std::string &name) const {
        return glGetUniformLocation(this->ID, name.c_str()) != -1;
    }
};

struct CoreVertexAttributes {
    unsigned int VBO = 0;
    unsigned int VAO = 0;
    std::optional<unsigned int> EBO = std::nullopt;
    std::optional<std::vector<unsigned int>> indices = std::nullopt;
    unsigned int elementCount = 0;
};

enum class ProjectionType {
    Orthographic,
    Perspective,
};

struct CoreObject {
    int id = 0;
    std::vector<CoreVertex> vertices;
    CoreVertexAttributes attributes;
    std::vector<CoreShader> shaders;
    std::optional<CoreShader> vertexShader;
    std::optional<CoreShader> fragmentShader;
    std::optional<CoreShaderProgram> program;
    std::vector<Texture> textures;
    bool visualizeTexture = false;
    ProjectionType projectionType = ProjectionType::Perspective;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    Material material;

    bool hidden = false;

    inline void hide() { this->hidden = true; }
    inline void show() { this->hidden = false; }

    void initialize();
    void initCore();
    void registerObject();
    void provideIndexedDrawing(std::vector<unsigned int> indices);
    void provideVertexData(std::vector<CoreVertex> vertices);
    void provideTextureCoords(std::vector<Size2d> textureCoords);
    void provideNormals(std::vector<Size3d> normals);
    void provideColors(std::vector<Color> colors);
    void addTexture(Texture texture);
    void setVertexColor(int index, Color color);
    void setObjectAlpha(float alpha);

    void enableTexturing();
    void disableTexturing();

    void translate(float x, float y, float z);
    void rotate(float angle, Axis axis = Axis::Z);
    void scale(float x, float y, float z);

    void updateProjectionType(ProjectionType type);

    CoreObject copy();

    std::vector<float> makeVertexData() const;
    std::vector<CoreShader> makeShaderList() const;

    CoreObject(std::vector<CoreVertex> vertices = {});
};

using RenderingFn = std::function<void(CoreObject *)>;

struct Renderer {
    std::vector<RenderingFn> dispatchers;
    std::vector<CoreObject *> registeredObjects;
    std::vector<CoreObject *> postRegisteredObjects;
    std::vector<RenderingFn> postDispatchers;

    static Renderer &instance() {
        static Renderer inst;
        return inst;
    }

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void registerObject(CoreObject *object, RenderingFn dispatcher,
                        bool postObject = false);

    void dispatchAll();
    void postDispatchAll();

  private:
    Renderer() = default;
};

void checkGLError(const std::string &operation);

CoreObject generateCubeObject(Position3d position, Size3d size);

extern GLuint defaultTexture;

GLuint getDefaultTexture();

#define CHECK_ERROR                                                            \
    do {                                                                       \
        GLenum error = glGetError();                                           \
        if (error != GL_NO_ERROR) {                                            \
            std::cerr << "OpenGL error: " << error << std::endl;               \
            throw std::runtime_error("OpenGL error occurred");                 \
        }                                                                      \
    } while (0)

CoreObject presentFullScreenTexture(Texture texture);

#endif // ATLAS_RENDERING_HPP
