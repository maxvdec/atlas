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
     * @brief Base color contribution of the surface.
     */
    Color albedo = {1.0, 1.0, 1.0, 1.0};
    /**
     * @brief Metallic factor controlling how conductive the material behaves.
     */
    float metallic = 0.0f;
    /**
     * @brief Roughness factor influencing the spread of specular highlights.
     */
    float roughness = 0.5f;
    /**
     * @brief Ambient occlusion term used to darken creases and cavities.
     */
    float ao = 1.0f;
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

    /**
     * @brief The tangent vector of the vertex, used for normal mapping and
     * parallax calculations.
     *
     */
    Normal3d tangent = {0.0, 0.0, 0.0};
    /**
     * @brief The bitangent vector of the vertex, used for normal mapping and
     * parallax calculations.
     *
     */
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
 * @brief Structure representing a single instance of an object for instanced
 * rendering. Each instance has its own position, rotation, and scale.
 *
 */
struct Instance {
    /**
     * @brief The position of this instance in 3D space.
     *
     */
    Position3d position = {0.0, 0.0, 0.0};
    /**
     * @brief The rotation of this instance in 3D space.
     *
     */
    Rotation3d rotation = {0.0, 0.0, 0.0};
    /**
     * @brief The scale of this instance in 3D space.
     *
     */
    Scale3d scale = {1.0, 1.0, 1.0};

  private:
    glm::mat4 model = glm::mat4(1.0f);

  public:
    /**
     * @brief Updates the model matrix based on the instance's position,
     * rotation, and scale.
     *
     */
    void updateModelMatrix();
    /**
     * @brief Gets the current model matrix for this instance.
     *
     * @return (glm::mat4) The model matrix.
     */
    glm::mat4 getModelMatrix() const { return model; }
    /**
     * @brief Moves the instance by a delta position.
     *
     * @param deltaPosition The amount to move by.
     */
    void move(const Position3d &deltaPosition);
    /**
     * @brief Sets the position of the instance.
     *
     * @param newPosition The new position to set.
     */
    void setPosition(const Position3d &newPosition);
    /**
     * @brief Sets the rotation of the instance.
     *
     * @param newRotation The new rotation to set.
     */
    void setRotation(const Rotation3d &newRotation);
    /**
     * @brief Rotates the instance by a delta rotation.
     *
     * @param deltaRotation The amount to rotate by.
     */
    void rotate(const Rotation3d &deltaRotation);
    /**
     * @brief Sets the scale of the instance.
     *
     * @param newScale The new scale to set.
     */
    void setScale(const Scale3d &newScale);
    /**
     * @brief Scales the instance by a delta scale factor.
     *
     * @param deltaScale The scale factor to multiply by.
     */
    void scaleBy(const Scale3d &deltaScale);

    /**
     * @brief Compares two instances for equality.
     *
     * @param other The other instance to compare with.
     * @return (bool) True if instances are equal, false otherwise.
     */
    bool operator==(const Instance &other) const {
        return position == other.position && rotation == other.rotation &&
               scale == other.scale;
    }
};

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
     * @brief Vector of instances for instanced rendering. Multiple copies of
     * the object can be rendered with different transforms efficiently.
     *
     */
    std::vector<Instance> instances;

    /**
     * @brief Function to construct a new CoreObject.
     *
     */
    CoreObject();

    /**
     * @brief Makes the object emissive by attaching a light to it.
     *
     * @param scene The scene to add the light to.
     * @param emissionColor The color of the emitted light.
     * @param intensity The intensity of the emitted light.
     */
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
    /**
     * @brief Binds an already compiled shader program for this object.
     */
    void attachProgram(const ShaderProgram &program) override;
    /**
     * @brief Builds a shader program from the supplied vertex and fragment
     * sources, then binds it.
     */
    void createAndAttachProgram(VertexShader &vertexShader,
                                FragmentShader &fragmentShader) override;
    /**
     * @brief Adds a texture to the object and makes it available during
     * rendering.
     */
    void attachTexture(const Texture &texture) override;
    /**
     * @brief Performs deferred setup of buffers, shaders, and state.
     */
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
    /**
     * @brief Sets a solid color override that multiplies with textures.
     */
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

    /**
     * @brief Places the object at an absolute position in world space.
     */
    void setPosition(const Position3d &newPosition) override;
    /**
     * @brief Moves the object relative to its current position.
     */
    void move(const Position3d &deltaPosition) override;
    /**
     * @brief Assigns an absolute rotation to the object.
     */
    void setRotation(const Rotation3d &newRotation) override;
    /**
     * @brief Rotates the object so its forward vector points towards a
     * target.
     */
    void lookAt(const Position3d &target,
                const Normal3d &up = {0.0, 1.0, 0.0}) override;
    /**
     * @brief Applies an incremental rotation around the object's axes.
     */
    void rotate(const Rotation3d &deltaRotation) override;
    /**
     * @brief Uniformly scales the object along each axis.
     */
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

    /**
     * @brief Attaches a physics body so the object can interact with the rigid
     * body system.
     */
    void setupPhysics(Body body) override;

    /**
     * @brief The light attached to this object if it's emissive. Used for
     * emissive objects created with makeEmissive().
     *
     */
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

    /**
     * @brief Whether the object should use deferred rendering. When false, the
     * object is rendered in the forward rendering pass.
     *
     */
    bool useDeferredRendering = true;

    void disableDeferredRendering() {
        useDeferredRendering = false;
        this->shaderProgram = ShaderProgram::fromDefaultShaders(
            AtlasVertexShader::Main, AtlasFragmentShader::Main);
    }

    /**
     * @brief Creates and returns a new instance for instanced rendering.
     *
     * @return (Instance&) Reference to the newly created instance.
     */
    inline Instance &createInstance() {
        instances.emplace_back();
        Instance &inst = instances.back();

        inst.position = this->position;
        inst.rotation = this->rotation;
        inst.scale = this->scale;
        inst.updateModelMatrix();

        return inst;
    }

  private:
    BufferIndex vbo;
    BufferIndex vao;
    BufferIndex ebo;
    BufferIndex instanceVBO = 0;

    std::vector<Instance> savedInstances;

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

    void updateInstances();

  public:
    void render(float dt) override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

    inline std::optional<ShaderProgram> getShaderProgram() override {
        return this->shaderProgram;
    }

    inline void setShader(const ShaderProgram &shader) override {
        this->shaderProgram = shader;
        this->initialize();
    }

    inline Position3d getPosition() const override { return position; }

    inline std::vector<CoreVertex> getVertices() const override {
        return vertices;
    }

    inline Size3d getScale() const override { return scale; }
    inline bool canCastShadows() const override { return castsShadows; }

    /**
     * @brief Performs per-frame updates such as component ticking or buffering
     * synchronization.
     */
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

    /**
     * @brief Assigns an absolute rotation to every mesh in the model.
     */
    inline void setRotation(const Rotation3d &newRotation) override {
        for (auto &obj : objects) {
            obj->setRotation(newRotation);
        }
    }

    /**
     * @brief Applies a texture to all contained CoreObjects.
     */
    inline void attachTexture(const Texture &texture) override {
        for (auto &obj : objects) {
            obj->attachTexture(texture);
        }
    }

    /**
     * @brief Scales every mesh in the model uniformly.
     */
    inline void setScale(const Scale3d &newScale) override {
        for (auto &obj : objects) {
            obj->setScale(newScale);
        }
    }

    /**
     * @brief Stores the current view matrix for every sub-object.
     */
    inline void setViewMatrix(const glm::mat4 &view) override {
        for (auto &obj : objects) {
            obj->setViewMatrix(view);
        }
    }

    /**
     * @brief Renders each CoreObject that composes the model.
     */
    inline void render(float dt) override {
        for (auto &component : components) {
            component->update(dt);
        }
        for (auto &obj : objects) {
            obj->render(dt);
        }
    }

    /**
     * @brief Updates all underlying CoreObjects to keep transforms and
     * animations synchronized.
     */
    inline void update(Window &window) override {
        for (auto &obj : objects) {
            obj->update(window);
        }
    }

    /**
     * @brief Initializes every CoreObject and attached component within the
     * model.
     */
    inline void initialize() override {
        for (auto &component : components) {
            component->init();
        }
        for (auto &obj : objects) {
            obj->initialize();
        }
    }

    /**
     * @brief Sets the projection matrix for all meshes composing the model.
     */
    inline void setProjectionMatrix(const glm::mat4 &projection) override {
        for (auto &obj : objects) {
            obj->setProjectionMatrix(projection);
        }
    }

    /**
     * @brief Forces every mesh to use the provided shader program.
     */
    inline void setShader(const ShaderProgram &shader) override {
        for (auto &obj : objects) {
            obj->setShader(shader);
        }
    }

    /**
     * @brief Retrieves the shader program currently bound to the first mesh.
     */
    inline std::optional<ShaderProgram> getShaderProgram() override {
        if (objects.empty()) {
            throw std::runtime_error("Model has no objects.");
        }
        return objects[0]->getShaderProgram().value();
    }

    /**
     * @brief Returns the world-space position of the first mesh in the model.
     */
    inline Position3d getPosition() const override {
        if (objects.empty()) {
            throw std::runtime_error("Model has no objects.");
        }
        return objects[0]->getPosition();
    }

    Model() = default;

    /**
     * @brief The material properties shared by all objects in the model.
     *
     */
    Material material;

    /**
     * @brief Whether the model should use deferred rendering. When false, the
     * model is rendered in the forward rendering pass.
     *
     */
    bool useDeferredRendering = true;

    /**
     * @brief Checks if the model can use deferred rendering.
     *
     * @return (bool) True if deferred rendering is enabled, false otherwise.
     */
    bool canUseDeferredRendering() const override {
        return useDeferredRendering;
    }

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
