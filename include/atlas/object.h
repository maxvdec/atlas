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
#include <any>
#include <memory>
#include <string>
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

class Scene;
class Light;

/**
 * @brief Alias that represents a texture coordinate in 2D space.
 *
 */
typedef std::array<double, 2> TextureCoordinate;

/**
 * @brief Structure representing the material properties of an object. Based on
 * Phong reflection model.
 *
 */
struct Material {
    /**
     * @brief The ambient color of the material. This is the color that the
     * material appears to be under ambient lighting.
     *
     */
    Color ambient = Color::white();
    /**
     * @brief The diffuse color of the material. This is the color that the
     * material appears to be under diffuse lighting.
     *
     */
    Color diffuse = Color::white();
    /**
     * @brief The specular color of the material. This is the color that the
     * material appears to be under specular lighting.
     *
     */
    Color specular = Color::white();
    /**
     * @brief The shininess of the material. This determines the size and
     * intensity of specular highlights. \warning The values must be greater
     * than 0.
     */
    float shininess = 32.0f;

    float reflectivity = 0.f; // 0.0 = no reflection, 1.0 = full reflection
};

/**
 * @brief Structure representing a single vertex in 3D space, including
 * position, color, texture coordinates, and normal vector. \warning This is
 * meant for internal use only and should not be used directly because it's
 * difficult to read and maintain.
 * \subsection core-vertex-example Example
 * ```cpp
 * // Create a vertex at position (1, 2, 3) with red color
 * CoreVertex vertex({1.0, 2.0, 3.0}, {1.0, 0.0, 0.0, 1.0}, {0.5, 0.5}, {0.0,
 * 0.0, 1.0});
 * ```
 */
struct CoreVertex {
    /**
     * @brief The position of the vertex in 3D space.
     *
     */
    Position3d position;
    /**
     * @brief The color of the vertex.
     *
     */
    Color color = {1.0, 1.0, 1.0, 1.0};
    /**
     * @brief The texture coordinates of the vertex.
     *
     */
    TextureCoordinate textureCoordinate = {0.0, 0.0};
    /**
     * @brief The normal vector of the vertex, used for lighting calculations.
     *
     */
    Normal3d normal = {0.0, 0.0, 0.0};

    Normal3d tangent = {0.0, 0.0, 0.0};
    Normal3d bitangent = {0.0, 0.0, 0.0};

    /**
     * @brief Function that constructs a new CoreVertex object.
     *
     * @param pos The position of the vertex in 3D space.
     * @param col The color of the vertex.
     * @param tex The texture coordinates of the vertex.
     * @param n The normal vector of the vertex.
     */
    CoreVertex(Position3d pos = {0.0, 0.0, 0.0},
               Color col = {1.0, 1.0, 1.0, 1.0},
               TextureCoordinate tex = {0.0, 0.0}, Normal3d n = {0.0, 0.0, 0.0},
               Normal3d t = {0.0, 0.0, 0.0}, Normal3d b = {0.0, 0.0, 0.0})
        : position(pos), color(col), textureCoordinate(tex), normal(n),
          tangent(t), bitangent(b) {}

    /**
     * @brief Gets the layout descriptors for the CoreVertex structure.
     *
     * @return (std::vector<LayoutDescriptor>) The layout descriptors.
     */
    static std::vector<LayoutDescriptor> getLayoutDescriptors();
};

/**
 * @brief Represents a buffer index.
 *
 */
typedef unsigned int BufferIndex;
/**
 * @brief Represents an index in a buffer.
 *
 */
typedef unsigned int Index;

class Window;

/**
 * @brief Represents a 3D object in the scene, including its geometry, material
 * and every interaction with the scene that it can have. It inherits from \ref
 * GameObject and can have \ref Component classes attached to it for extended
 * functionality.
 *
 * \subsection core-object-example Example of a CoreObject
 * ```cpp
 * // Create a simple cube object
 * CoreObject cube = createBox({1.0, 1.0, 1.0}, Color::red());
 * // Set the position of the cube
 * cube.setPosition({0.0, 0.5, 0.0});
 * // Attach a texture to the cube
 * Texture brickTexture("path/to/brick_texture.png");
 * cube.attachTexture(brickTexture);
 * // Create and attach a shader program to the cube
 * VertexShader vertexShader("path/to/vertex_shader.glsl");
 * FragmentShader fragmentShader("path/to/fragment_shader.glsl");
 * cube.createAndAttachProgram(vertexShader, fragmentShader);
 * // Set the material properties of the cube
 * Material cubeMaterial;
 * cubeMaterial.ambient = Color(0.2, 0.2, 0.2, 1.0);
 * cubeMaterial.diffuse = Color(0.8, 0.0, 0.0, 1.0);
 * cubeMaterial.specular = Color(1.0, 1.0, 1.0, 1.0);
 * cubeMaterial.shininess = 32.0f;
 * cube.material = cubeMaterial;
 * // Add a physics body to the cube
 * Body cubeBody;
 * cubeBody.type = BodyType::Dynamic;
 * cubeBody.mass = 1.0f;
 * cubeBody.friction = 0.5f;
 * cube.setupPhysics(cubeBody);
 * // Add the cube to the scene
 * scene.addObject(&cube);
 * ```
 *
 */
class CoreObject : public GameObject {
  public:
    /**
     * @brief The vertices of the object.
     *
     */
    std::vector<CoreVertex> vertices;
    /**
     * @brief The indices of the object. These indicate how the vertices are
     * connected to form faces.
     *
     */
    std::vector<Index> indices;
    /**
     * @brief The shader program used by the object.
     *
     */
    ShaderProgram shaderProgram;
    /**
     * @brief The textures applied to the object.
     *
     */
    std::vector<Texture> textures;
    /**
     * @brief The material properties of the object.
     *
     */
    Material material = Material();

    /**
     * @brief Function to construct a new CoreObject.
     *
     */
    CoreObject();

    void makeEmissive(Scene *scene, Color emissionColor, float intensity);

    /**
     * @brief Function to attach vertices and update the object's vertex buffer.
     *
     * @param newVertices The vertices to attach.
     */
    void attachVertices(const std::vector<CoreVertex> &newVertices);
    /**
     * @brief Function to attach indices and update the object's index buffer.
     *
     * @param newIndices The indices to attach.
     */
    void attachIndices(const std::vector<Index> &newIndices);
    void attachProgram(const ShaderProgram &program) override;
    void createAndAttachProgram(VertexShader &vertexShader,
                                FragmentShader &fragmentShader) override;
    void attachTexture(const Texture &texture) override;
    void initialize() override;

    /**
     * @brief Function to render the object with color and texture.
     *
     */
    void renderColorWithTexture();
    /**
     * @brief Function to render the object with only its color.
     *
     */
    void renderOnlyColor();
    /**
     * @brief Function to render the object with only its texture.
     *
     */
    void renderOnlyTexture();
    void setColor(const Color &color) override;

    /**
     * @brief The position of the object in 3D space.
     *
     */
    Position3d position = {0.0, 0.0, 0.0};
    /**
     * @brief The rotation of the object in 3D space.
     *
     */
    Rotation3d rotation = {0.0, 0.0, 0.0};
    /**
     * @brief The scale of the object in 3D space.
     *
     */
    Scale3d scale = {1.0, 1.0, 1.0};

    void setPosition(const Position3d &newPosition) override;
    void move(const Position3d &deltaPosition) override;
    void setRotation(const Rotation3d &newRotation) override;
    void lookAt(const Position3d &target,
                const Normal3d &up = {0.0, 1.0, 0.0}) override;
    void rotate(const Rotation3d &deltaRotation) override;
    void setScale(const Scale3d &newScale) override;

    /**
     * @brief Function that updates the model matrix based on the object's
     * position, rotation, and scale.
     *
     */
    void updateModelMatrix();
    /**
     * @brief Function that updates the object's vertex buffer.
     *
     */
    void updateVertices();

    /**
     * @brief Function that creates a copy of the object.
     *
     * @return (CoreObject) The cloned object.
     */
    CoreObject clone() const;

    inline void show() override { isVisible = true; }
    inline void hide() override { isVisible = false; }

    void setupPhysics(Body body) override;

    std::shared_ptr<Light> light = nullptr;

    /**
     * @brief The unique identifier for the object.
     *
     */
    Id id;

    /**
     * @brief Variable that determines if the object casts shadows.
     *
     */
    bool castsShadows = true;

    /**
     * @brief Function that adds a component to the object. The component will
     * have this CoreObject as its parent.
     *
     * @tparam T The type of component to add.
     * @param existing The existing component to add.
     */
    template <typename T>
        requires std::is_base_of_v<Component, T>
    void addComponent(T existing) {
        std::shared_ptr<T> component = std::make_shared<T>(existing);
        component->object = this;
        component->body = this->body.get();
        components.push_back(component);
    }

    bool useDeferredRendering = true;

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

    bool canUseDeferredRendering() const override {
        return useDeferredRendering;
    }
};

/**
 * @brief Function that creates a box CoreObject with the specified size and
 * color.
 *
 * @param size The size of the box in 3D space.
 * @param color The color of the box.
 * @return (CoreObject) The created box object.
 */
CoreObject createBox(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a box CoreObject with a default texture and
 * physics body.
 *
 * @param size The size of the box in 3D space.
 * @return (CoreObject) The created debug box object.
 */
CoreObject createDebugBox(Size3d size);
/**
 * @brief Function that creates a plane CoreObject with the specified size and
 * color.
 *
 * @param size The size of the plane in 2D space.
 * @param color The color of the plane.
 * @return (CoreObject) The created plane object.
 */
CoreObject createPlane(Size2d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a debug plane CoreObject with the specified size
 * and attaches a default texture.
 *
 * @param size The size of the plane in 2D space.
 * @return (CoreObject) The created debug plane object.
 */
CoreObject createDebugPlane(Size2d size);
/**
 * @brief Function that creates a pyramid CoreObject with the specified size and
 * color.
 *
 * @param size The size of the pyramid in 3D space.
 * @param color The color of the pyramid.
 * @return (CoreObject) The created pyramid object.
 */
CoreObject createPyramid(Size3d size, Color color = {1.0, 1.0, 1.0, 1.0});
/**
 * @brief Function that creates a sphere CoreObject with the specified radius,
 * sectorCount, stackCount and color.
 *
 * @param radius The radius of the sphere.
 * @param sectorCount The number of sectors (longitude divisions) of the sphere.
 * @param stackCount The number of stacks (latitude divisions) of the sphere.
 * @param color The color of the sphere.
 * @return (CoreObject) The created sphere object.
 */
CoreObject createSphere(double radius, unsigned int sectorCount = 36,
                        unsigned int stackCount = 18,
                        Color color = {1.0, 1.0, 1.0, 1.0});

/**
 * @brief Function that creates a debug sphere CoreObject with the specified
 * radius, sectorCount and stackCount, and attaches a default texture and
 * physics body.
 *
 * @param radius The radius of the sphere.
 * @param sectorCount The number of sectors (longitude divisions) of the
 * @param stackCount The number of stacks (latitude divisions) of the sphere.
 * @return (CoreObject) The created sphere object.
 */
CoreObject createDebugSphere(double radius, unsigned int sectorCount = 36,
                             unsigned int stackCount = 18);

/**
 * @brief Type that describes how particles are emitted.
 *
 */
enum class ParticleEmissionType {
    /**
     * @brief Emission type for fountain-like particle effects.
     *
     */
    Fountain,
    /**
     * @brief Emission type for snow-like particle effects.
     *
     */
    Ambient
};

/**
 * @brief Structure representing settings for particle behavior and appearance.
 *
 */
struct ParticleSettings {
    /**
     * @brief The minimum lifetime of a particle in seconds.
     *
     */
    float minLifetime = 1.0f;
    /**
     * @brief The maximum lifetime of a particle in seconds.
     *
     */
    float maxLifetime = 3.0f;
    /**
     * @brief The minimum size of a particle.
     *
     */
    float minSize = 0.02f;
    /**
     * @brief The maximum size of a particle.
     *
     */
    float maxSize = 0.01f;
    /**
     * @brief The speed at which a particle fades out.
     *
     */
    float fadeSpeed = 0.5f;
    /**
     * @brief The gravitational force applied to particles.
     *
     */
    float gravity = -9.81f;
    /**
     * @brief The spread of particles from the emitter.
     *
     */
    float spread = 1.0f;
    /**
     * @brief The speed variation of particles. Is how much the speed is
     * randomized.
     *
     */
    float speedVariation = 1.0f;
};

/**
 * @brief Structure representing a single particle in a particle system.
 *
 */
struct Particle {
    /**
     * @brief The position of the particle in 3D space.
     *
     */
    Position3d position;
    /**
     * @brief The velocity of the particle in 3D space.
     *
     */
    Magnitude3d velocity;
    /**
     * @brief The color that the particle will have.
     *
     */
    Color color;
    /**
     * @brief The current life of the particle in seconds.
     *
     */
    float life;
    /**
     * @brief The maximum life of the particle in seconds.
     *
     */
    float maxLife;
    /**
     * @brief The scale the particle will have.
     *
     */
    float size;
    /**
     * @brief Whether the particle is active or not.
     *
     */
    bool active;
};

/**
 * @brief Class representing a particle emitter in the game. It emits and
 * manages particles.
 *
 * \subsection emitter-example Example
 * ```cpp
 * // Create a particle emitter with a maximum of 200 particles
 * ParticleEmitter emitter(200);
 * // Set the position of the emitter
 * emitter.setPosition({0.0f, 0.0f, 0.0f});
 * // Set the emission type to Fountain
 * emitter.setEmissionType(ParticleEmissionType::Fountain);
 * // Set the direction of particle emission
 * emitter.setDirection({0.0f, 1.0f, 0.0f});
 * // Set the spawn radius of particles
 * emitter.setSpawnRadius(0.5f);
 * // Set the spawn rate to 20 particles per second
 * emitter.setSpawnRate(20.0f);
 * // Define particle settings
 * ParticleSettings settings;
 * settings.minLifetime = 1.0f;
 * settings.maxLifetime = 3.0f;
 * settings.minSize = 0.05f;
 * settings.maxSize = 0.1f;
 * settings.fadeSpeed = 0.5f;
 * settings.gravity = -9.81f;
 * settings.spread = 1.0f;
 * settings.speedVariation = 0.5f;
 * emitter.setParticleSettings(settings);
 * // Attach a texture to the particles
 * Texture particleTexture("path/to/particle_texture.png");
 * emitter.attachTexture(particleTexture);
 * emitter.enableTexture();
 * // Add the emitter to the scene
 * scene.addObject(&emitter);
 * ```
 *
 */
class ParticleEmitter : public GameObject {
  public:
    void initialize() override;
    void render(float dt) override;
    void update(Window &window) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;
    void setViewMatrix(const glm::mat4 &view) override;

    /**
     * @brief Constructs a new ParticleEmitter object.
     *
     * @param maxParticles The maximum number of particles the emitter can
     * handle.
     */
    ParticleEmitter(unsigned int maxParticles = 100);

    void attachTexture(const Texture &tex) override;
    void setColor(const Color &color) override;
    /**
     * @brief Function that enables the use of a texture for the particles.
     *
     */
    inline void enableTexture() { useTexture = true; };
    /**
     * @brief Function that disables the use of a texture for the particles.
     *
     */
    inline void disableTexture() { useTexture = false; };

    void setPosition(const Position3d &newPosition) override;
    void move(const Position3d &deltaPosition) override;
    inline Position3d getPosition() const override { return position; };
    inline bool canCastShadows() const override { return false; };

    /**
     * @brief Function that sets the type of particle emission.
     *
     * @param type The type of particle emission.
     */
    void setEmissionType(ParticleEmissionType type);
    /**
     * @brief The direction in which particles are emitted.
     *
     * @param dir The direction vector.
     */
    void setDirection(const Magnitude3d &dir);
    /**
     * @brief The radius around the emitter from which particles are spawned.
     *
     * @param radius The spawn radius.
     */
    void setSpawnRadius(float radius);
    /**
     * @brief How much particles are spawned each second.
     *
     * @param particlesPerSecond The number of particles to spawn per second.
     */
    void setSpawnRate(float particlesPerSecond);
    /**
     * @brief Sets the settings for the particles.
     *
     * @param settings The particle settings to apply.
     */
    void setParticleSettings(const ParticleSettings &settings);

    /**
     * @brief Emits particles once.
     *
     */
    void emitOnce();
    /**
     * @brief Emits particles continuously.
     *
     */
    void emitContinuously();
    /**
     * @brief Starts emitting particles.
     *
     */
    void startEmission();
    /**
     * @brief Stops emitting particles.
     *
     */
    void stopEmission();
    /**
     * @brief Emits a burst of particles.
     *
     * @param count The number of particles to emit in the burst.
     */
    void emitBurst(int count);

    /**
     * @brief Sets the spawn rate for the particles.
     *
     * @param rate The spawn rate in particles per second.
     */
    inline void setSpawnRate(int rate) {
        setSpawnRate(static_cast<float>(rate));
    }

  private:
    std::vector<Particle> particles;
    unsigned int maxParticles;
    unsigned int activeParticleCount = 0;

    ParticleEmissionType emissionType = ParticleEmissionType::Fountain;
    Magnitude3d direction = {0.0, 1.0, 0.0};
    float spawnRadius = 0.1f;
    float spawnRate = 10.0f;
    ParticleSettings settings;

    float timeSinceLastEmission = 0.0f;
    bool isEmitting = true;
    bool doesEmitOnce = false;
    bool hasEmittedOnce = false;
    int burstCount = 0;

    Id vao, vbo;
    ShaderProgram program;
    Texture texture;
    Color color = Color::white();
    bool useTexture = false;

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);

    Position3d position = {0.0, 0.0, 0.0};
    std::optional<Position3d> firstCameraPosition = std::nullopt;

    void spawnParticle();
    void updateParticle(Particle &p, float deltaTime);
    Magnitude3d generateRandomVelocity();
    Position3d generateSpawnPosition();
    int findInactiveParticle();
    void activateParticle(int index);
};

class Resource;
class aiNode;
class aiScene;
class aiMesh;
class aiMaterial;
/**
 * @brief Class that represents a 3D model composed of multiple CoreObjects. It
 * can be loaded from a resource file and manages its constituent objects.
 * \subsection model-example Example
    * ```cpp
    * // Load a 3D model from a resource file
    * Resource modelResource =
 Workspace::get().createResource("path/to/model.obj");
    * Model myModel;
    * myModel.fromResource(modelResource);
    * // Set the position of the model in the scene
    * myModel.setPosition({0.0f, 0.0f, 0.0f});
    * // Scale the model to twice its original size
    * myModel.setScale({2.0f, 2.0f, 2.0f});
    * // Rotate the model 45 degrees around the Y-axis
    * myModel.setRotation({0.0f, 45.0f, 0.0f});
    * // Add the model to the scene
    * window.addObject(&myModel);
    * ```
 *
 */
class Model : public GameObject {
  public:
    /**
     * @brief Loads the model from a resource.
     *
     * @param resource The resource to load the model from.
     */
    void fromResource(Resource resource);

    /**
     * @brief Gets the objects that make up the model.
     *
     * @return (std::vector<std::shared_ptr<CoreObject>>) The objects.
     */
    inline std::vector<std::shared_ptr<CoreObject>> getObjects() {
        return objects;
    }

    /**
     * @brief Moves the model by a certain amount.
     *
     * @param deltaPosition The amount to move the model by.
     */
    inline void move(const Position3d &deltaPosition) override {
        for (auto &obj : objects) {
            obj->move(deltaPosition);
        }
    }

    /**
     * @brief Set the Position object
     *
     * @param newPosition The new position to set.
     */
    inline void setPosition(const Position3d &newPosition) override {
        for (auto &obj : objects) {
            obj->setPosition(newPosition);
        }
    }

    inline void setRotation(const Rotation3d &newRotation) override {
        for (auto &obj : objects) {
            obj->setRotation(newRotation);
        }
    }

    inline void attachTexture(const Texture &texture) override {
        for (auto &obj : objects) {
            obj->attachTexture(texture);
        }
    }

    inline void setScale(const Scale3d &newScale) override {
        for (auto &obj : objects) {
            obj->setScale(newScale);
        }
    }

    inline void setViewMatrix(const glm::mat4 &view) override {
        for (auto &obj : objects) {
            obj->setViewMatrix(view);
        }
    }

    inline void render(float dt) override {
        for (auto &component : components) {
            component->update(dt);
        }
        for (auto &obj : objects) {
            obj->render(dt);
        }
    }

    inline void update(Window &window) override {
        for (auto &obj : objects) {
            obj->update(window);
        }
    }

    inline void initialize() override {
        for (auto &component : components) {
            component->init();
        }
        for (auto &obj : objects) {
            obj->initialize();
        }
    }

    inline void setProjectionMatrix(const glm::mat4 &projection) override {
        for (auto &obj : objects) {
            obj->setProjectionMatrix(projection);
        }
    }

    inline void setShader(const ShaderProgram &shader) override {
        for (auto &obj : objects) {
            obj->setShader(shader);
        }
    }

    inline std::optional<ShaderProgram> getShaderProgram() override {
        if (objects.empty()) {
            throw std::runtime_error("Model has no objects.");
        }
        return objects[0]->getShaderProgram().value();
    }

    inline Position3d getPosition() const override {
        if (objects.empty()) {
            throw std::runtime_error("Model has no objects.");
        }
        return objects[0]->getPosition();
    }

    Model() = default;

    Material material;

  private:
    std::vector<std::shared_ptr<CoreObject>> objects;
    std::string directory;

    void loadModel(Resource resource);
    void processNode(aiNode *node, const aiScene *scene,
                     glm::mat4 parentTransform = glm::mat4(1.0f));
    CoreObject processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, std::any type,
                                              const std::string &typeName);
};

#endif // ATLAS_OBJECT_H
