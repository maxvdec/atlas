/*
 object.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Object properties and definitions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_OBJECT_H
#define ATLAS_OBJECT_H

#include "atlas/component.h"
#include "bezel/body.h"
#include <memory>
#include <type_traits>
#pragma once

#include "atlas/core/shader.h"
#include "atlas/core/renderable.h"
#include "atlas/texture.h"
#include <array>
#include <atlas/units.h>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

typedef std::array<double, 2> TextureCoordinate;

struct Material {
    Color ambient = Color::white();
    Color diffuse = Color::white();
    Color specular = Color::white();
    float shininess = 32.0f;
};

struct CoreVertex {
    Position3d position;
    Color color = {1.0, 1.0, 1.0, 1.0};
    TextureCoordinate textureCoordinate = {0.0, 0.0};
    Normal3d normal = {0.0, 0.0, 0.0};

    CoreVertex(Position3d pos = {0.0, 0.0, 0.0},
               Color col = {1.0, 1.0, 1.0, 1.0},
               TextureCoordinate tex = {0.0, 0.0}, Normal3d n = {0.0, 0.0, 0.0})
        : position(pos), color(col), textureCoordinate(tex), normal(n) {}

    static std::vector<LayoutDescriptor> getLayoutDescriptors();
};

typedef unsigned int BufferIndex;
typedef unsigned int Index;

class Window;

class CoreObject : public GameObject {
  public:
    std::vector<CoreVertex> vertices;
    std::vector<Index> indices;
    ShaderProgram shaderProgram;
    std::vector<Texture> textures;
    Material material = Material();

    CoreObject();

    void attachVertices(const std::vector<CoreVertex> &newVertices);
    void attachIndices(const std::vector<Index> &newIndices);
    void attachProgram(const ShaderProgram &program) override;
    void createAndAttachProgram(VertexShader &vertexShader,
                                FragmentShader &fragmentShader) override;
    void attachTexture(const Texture &texture) override;
    void initialize() override;

    void renderColorWithTexture();
    void renderOnlyColor();
    void renderOnlyTexture();
    void setColor(const Color &color) override;

    Position3d position = {0.0, 0.0, 0.0};
    Rotation3d rotation = {0.0, 0.0, 0.0};
    Scale3d scale = {1.0, 1.0, 1.0};

    void setPosition(const Position3d &newPosition) override;
    void move(const Position3d &deltaPosition) override;
    void setRotation(const Rotation3d &newRotation) override;
    void lookAt(const Position3d &target,
                const Normal3d &up = {0.0, 1.0, 0.0}) override;
    void rotate(const Rotation3d &deltaRotation) override;
    void setScale(const Scale3d &newScale) override;

    void updateModelMatrix();
    void updateVertices();

    CoreObject clone() const;

    inline void show() override { isVisible = true; }
    inline void hide() override { isVisible = false; }

    void setupPhysics(Body body) override;

    Id id;

    bool castsShadows = true;

    template <typename T>
        requires std::is_base_of_v<Component, T>
    void addComponent(T existing) {
        std::shared_ptr<T> component = std::make_shared<T>(existing);
        component->object = this;
        component->body = this->body.get();
        components.push_back(component);
    }

  private:
    BufferIndex vbo;
    BufferIndex vao;
    BufferIndex ebo;

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    bool useColor = true;
    bool useTexture = false;

    bool isVisible = true;

    bool hasPhysics = false;

    friend class Window;
    friend class RenderTarget;
    friend class Skybox;

    std::vector<std::shared_ptr<Component>> components;

  public:
    void render(float dt) override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    inline std::optional<ShaderProgram> getShaderProgram() override {
        return this->shaderProgram;
    }

    inline void setShader(const ShaderProgram &shader) override {
        this->shaderProgram = shader;
    }

    inline Position3d getPosition() const override { return position; }

    inline std::vector<CoreVertex> getVertices() const override {
        return vertices;
    }

    inline Size3d getScale() const override { return scale; }
    inline bool canCastShadows() const override { return castsShadows; }

    void update(Window &window) override;
};

CoreObject createBox(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
CoreObject createDebugBox(Size3d size);
CoreObject createPlane(Size2d size, Color color = {1.0, 1.0, 1.0, 1.0});
CoreObject createDebugPlane(Size2d size);
CoreObject createPyramid(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
CoreObject createSphere(double radius, unsigned int sectorCount = 36,
                        unsigned int stackCount = 18,
                        Color color = {1.0, 1.0, 1.0, 1.0});

CoreObject createDebugSphere(double radius, unsigned int sectorCount = 36,
                             unsigned int stackCount = 18);
struct Particle {
    Position3d position;
    Magnitude3d velocity;
    Color color;
    float life;
    float size;
};

class ParticleEmitter : public Component {
  public:
    void init() override;
    void update(float dt) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;
    void setViewMatrix(const glm::mat4 &view) override;

    ParticleEmitter(unsigned int maxParticles = 100);

  private:
    std::vector<Particle> particles;
    unsigned int maxParticles;

    Id vao, vbo;
    ShaderProgram program;

    void respawnParticle(Particle &p, const Position3d &emitterPos);
};

#endif // ATLAS_OBJECT_H
