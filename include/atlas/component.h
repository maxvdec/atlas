//
// component.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Component base class
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_COMPONENT_H
#define ATLAS_COMPONENT_H

#include "atlas/core/renderable.h"
#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "bezel/bezel.h"
#include "opal/opal.h"
#include <climits>
#include <memory>
#include <random>
#include <vector>

class CoreObject;
class Window;
class GameObject;

/**
 * @brief Behavior or property that can be attached to a \ref GameObject to
 * extend its capabilities.
 *
 * \subsection example-component Example of a component
 * ```cpp
 * // Define a custom component by inheriting from Component
 * class CustomComponent : public Component {
 *  public:
 *    void init() override {
 *      // Custom initialization code here
 *    }
 *    void update(float deltaTime) override {
 *      // Custom update code here
 *    }
 * };
 * ```
 */
class Component {
  public:
    virtual ~Component() = default;
    /**
     * @brief Initializes the component. This method is called once when the
     * component is added to a GameObject.
     *
     */
    virtual void init() {}
    virtual void beforePhysics() {}
    virtual void atAttach() {}
    /**
     * @brief Updates the component each frame. This method is called every
     * frame before rendering.
     *
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void update([[maybe_unused]] float deltaTime) {}
    /**
     * @brief Performs changes when the GameObject's view matrix is updated.
     *
     * @param view The new view matrix.
     */
    virtual void setViewMatrix([[maybe_unused]] const glm::mat4 &view) {}
    /**
     * @brief Performs changes when the GameObject's projection matrix is
     * updated.
     *
     * @param projection The new projection matrix.
     */
    virtual void
    setProjectionMatrix([[maybe_unused]] const glm::mat4 &projection) {}
    /**
     * @brief Gets the window associated with the component's GameObject.
     *
     * @return (Window*) The window instance.
     */
    Window *getWindow();

    /**
     * @brief Construct a new Component object
     *
     */
    Component() = default;

    /**
     * @brief Gets the GameObject associated with the component.
     *
     */
    GameObject *object = nullptr;
};

/**
 * @brief Component that is specifically tied to a certain type of \ref
 * GameObject. It cannot be added to objects that do not inherit from the
 * specified type.
 *
 * @tparam T The type of GameObject the component is tied to.
 */
template <typename T>
    requires std::is_base_of_v<GameObject, T>
class TraitComponent : public Component {
  public:
    /**
     * @brief Initializes the component. This method is called once when the
     * component is added to a GameObject.
     *
     */
    virtual void init() override {};
    /**
     * @brief Updates the component each frame. This method is called every
     * frame before rendering.
     *
     * @param deltaTime The time elapsed since the last frame.
     */
    void update([[maybe_unused]] float deltaTime) override {
        if (typedObject != nullptr) {
            updateComponent(typedObject);
        }
    }
    /**
     * @brief Updates the component with a typed reference to the GameObject it
     * is bound to.
     *
     * @param object A pointer to the typed GameObject.
     */
    virtual void updateComponent([[maybe_unused]] T *object) {}
    /**
     * @brief Sets the typed object reference for the component.
     *
     * @param obj A pointer to the typed GameObject.
     */
    inline void setTypedObject([[maybe_unused]] T *obj) { typedObject = obj; }

    /**
     * @brief Provides direct access to the specialized GameObject this trait
     * decorates.
     *
     * @return (T*) Pointer to the typed object, or nullptr if the component
     * has not been attached yet.
     */
    inline T *getObject() { return typedObject; }

  private:
    T *typedObject = nullptr;
};

/**
 * @brief Base class for all Game Objects. It extends from \ref Renderable and
 * it provides common functionality for all game objects in the scene.
 */
class GameObject : public Renderable {
  public:
    /**
     * @brief Construct a new GameObject and assign it a unique ID.
     */
    GameObject() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, INT_MAX);
        id = dist(gen);
    }

    /**
     * @brief Attaches a shader program to the object.
     *
     * @param program
     */
    virtual void attachProgram([[maybe_unused]] const ShaderProgram &program) {
    };
    /**
     * @brief Creates and attaches a shader program to the object.
     *
     * @param vertexShader The vertex shader for the program
     * @param fragmentShader The fragment shader for the program
     */
    virtual void
    createAndAttachProgram([[maybe_unused]] VertexShader &vertexShader,
                           [[maybe_unused]] FragmentShader &fragmentShader) {};
    /**
     * @brief Attaches a texture to the object.
     *
     * @param texture The texture to attach.
     */
    virtual void attachTexture([[maybe_unused]] const Texture &texture) {};
    /**
     * @brief Sets the color of the object.
     *
     * @param color The new color to set.
     */
    virtual void setColor([[maybe_unused]] const Color &color) {};
    /**
     * @brief Sets the position of the object.
     *
     * @param newPosition The new position to set.
     */
    virtual void setPosition([[maybe_unused]] const Position3d &newPosition) {};
    /**
     * @brief Moves the object by a certain amount.
     *
     * @param deltaPosition The amount to move the object by.
     */
    virtual void move([[maybe_unused]] const Position3d &deltaPosition) {};
    /**
     * @brief Sets the rotation of the object.
     *
     * @param newRotation The new rotation to set.
     */
    virtual void setRotation([[maybe_unused]] const Rotation3d &newRotation) {};
    /**
     * @brief Sets the object to look at a specific point in 3D space.
     *
     * @param target The point to look at.
     * @param up The up vector to define the orientation.
     */
    virtual void lookAt([[maybe_unused]] const Position3d &target,
                        [[maybe_unused]] const Normal3d &up) {};
    /**
     * @brief Rotates the object by a certain amount.
     *
     * @param deltaRotation The amount to rotate the object by.
     */
    virtual void rotate([[maybe_unused]] const Rotation3d &deltaRotation) {};
    /**
     * @brief Sets the scale of the object.
     *
     * @param newScale The new scale of the object.
     */
    virtual void setScale([[maybe_unused]] const Scale3d &newScale) {};
    /**
     * @brief Hides the object, making it invisible in the scene.
     *
     */
    virtual void hide() {};
    /**
     * @brief Shows the object, making it visible in the scene.
     *
     */
    virtual void show() {};

    /**
     * @brief Adds a component to the object.
     *
     * @tparam T The type of component to add.
     * @param existing The component instance to add. \warning It must be
     * long-lived. This means that declaring it as a class property is a good
     * idea.
     */
    template <typename T>
        requires std::is_base_of_v<Component, T>
    void addComponent(T &&existing) {
        std::shared_ptr<T> component =
            std::make_shared<T>(std::forward<T>(existing));
        component->object = this;
        component->atAttach();
        components.push_back(component);
    }

    void beforePhysics() override {
        for (auto &component : components) {
            component->beforePhysics();
        }
    }

    /**
     * @brief Adds a trait component to the object. A trait component is a
     * component that is tied to a specific type of GameObject.
     *
     * @tparam U The type of GameObject the trait component is tied to.
     * @tparam T The type of trait component to add.
     * @param existing The trait component instance to add. \warning It must be
     * long-lived. This means that declaring it as a class property is a good
     * idea.
     */
    template <typename U, typename T>
        requires std::is_base_of_v<TraitComponent<U>, T>
    void addTraitComponent(T &&existing) {
        if (static_cast<U *>(this) == nullptr) {
            throw std::runtime_error(
                "Cannot add TraitComponent to object that is not of the "
                "correct type.");
        }
        existing.setTypedObject(static_cast<U *>(this));
        std::shared_ptr<T> component =
            std::make_shared<T>(std::forward<T>(existing));
        component->object = this;
        components.push_back(component);
    }

    /**
     * @brief Gets the first component that matches the specified type.
     *
     * @tparam T The type of component to get.
     * @return (std::shared_ptr<T>) A shared pointer to the component if found,
     * or nullptr if not found.
     */
    template <typename T>
        requires std::is_base_of_v<Component, T>
    std::shared_ptr<T> getComponent() {
        for (auto &component : components) {
            std::shared_ptr<T> casted = std::dynamic_pointer_cast<T>(component);
            if (casted != nullptr) {
                return casted;
            }
        }
        return nullptr;
    }

    /**
     * @brief Returns the unique identifier associated with this object.
     */
    unsigned int getId() override { return id; }

    virtual Rotation3d getRotation() const { return Rotation3d(0.f, 0.f, 0.f); }

  protected:
    std::vector<std::shared_ptr<Component>> components;

    /**
     * @brief The unique identifier for the object.
     */
    int id;
};

/**
 * @brief A Compound object is a GameObject that can be extended by the user to
 * generate objects that encapsulate multiple CoreObject. This is useful for
 * creating complex objects that are made up of multiple simpler objects.
 *
 * \subsection compound-object-example Example of a compound object
 * ```cpp
 * class Car : public CompoundObject {
 *  public:
 *    void init() override {
 *      // Create and add car body
 *      CoreObject *body = new CoreObject();
 *      // Set up body properties...
 *      addObject(body);
 *    }
 *
 *    void updateObjects(Window &window) override {
 *      // Update car-specific logic, e.g., movement
 *    }
 * };
 * ```
 *
 */
class CompoundObject : public GameObject {
  public:
    /**
     * @brief The objects that make up the compound object.
     *
     */
    std::vector<GameObject *> objects;

    /**
     * @brief Bootstraps the compound object, initializing child objects and
     * dispatching component hooks.
     */
    virtual void initialize() override;

    /**
     * @brief Ticks the compound object, ensuring child objects stay
     * synchronized before rendering.
     */
    virtual void update(Window &window) override;
    /**
     * @brief Updates the objects within the compound object.
     *
     * @param window The window where the objects are being rendered.
     */
    virtual void updateObjects([[maybe_unused]] Window &window) {};
    /**
     * @brief Initializes the compound object.
     *
     */
    virtual void init() {};

    /**
     * @brief Renders every child CoreObject that composes the compound
     * structure.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Propagates the active view matrix to every child CoreObject.
     */
    void setViewMatrix(const glm::mat4 &view) override;
    /**
     * @brief Propagates the active projection matrix to every child
     * CoreObject.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override;
    /**
     * @brief Retrieves the shader program currently in use by the first
     * underlying CoreObject.
     */
    std::optional<std::shared_ptr<opal::Pipeline>> getPipeline() override;
    /**
     * @brief Forces all child objects to use the provided shader program.
     */
    void setPipeline(std::shared_ptr<opal::Pipeline> &pipeline) override;
    /**
     * @brief Obtains the position of the compound object based on its first
     * child.
     */
    Position3d getPosition() const override;
    /**
     * @brief Collects the vertices from the first child CoreObject for quick
     * queries such as bounding-box generation.
     */
    std::vector<CoreVertex> getVertices() const override;
    /**
     * @brief Reports the scale of the compound object, mirroring its first
     * child object.
     */
    Size3d getScale() const override;
    /**
     * @brief Indicates whether the compound object is eligible to cast
     * shadows.
     */
    bool canCastShadows() const override;
    /**
     * @brief Repositions the entire aggregate by offsetting all children the
     * same amount.
     */
    void setPosition(const Position3d &newPosition) override;
    /**
     * @brief Translates every child CoreObject by the supplied delta.
     */
    void move(const Position3d &deltaPosition) override;
    /**
     * @brief Applies an absolute rotation to all children, maintaining their
     * relative offsets.
     */
    void setRotation(const Rotation3d &newRotation) override;
    /**
     * @brief Rotates every child CoreObject around its own origin.
     */
    void lookAt(const Position3d &target, const Normal3d &up) override;
    /**
     * @brief Applies an incremental rotation to all children.
     */
    void rotate(const Rotation3d &deltaRotation) override;
    /**
     * @brief Uniformly rescales the compound object across all children.
     */
    void setScale(const Scale3d &newScale) override;
    /**
     * @brief Temporarily removes the aggregate from the render queue.
     */
    void hide() override;
    /**
     * @brief Makes the aggregate renderable again after being hidden.
     */
    void show() override;

    /**
     * @brief Adds a component to the object.
     *
     * @param obj The component instance to add. \warning It must be long-lived.
     * This means that declaring it as a class property is a good idea.
     */
    inline void addObject(GameObject *obj) {
        objects.push_back(obj);
        if (obj != nullptr && obj->renderLateForward) {
            lateForwardObjects.push_back(obj);
        }
    }

    /**
     * @brief Returns the late forward proxy renderable, if any children
     * require late rendering.
     */
    Renderable *getLateRenderable();

    bool canUseDeferredRendering() override;

  private:
    class LateCompoundRenderable;

    Position3d position{0.0, 0.0, 0.0};
    std::vector<Position3d> originalPositions;
    std::vector<GameObject *> lateForwardObjects;
    std::shared_ptr<LateCompoundRenderable> lateRenderableProxy;
    bool lateRenderableRegistered = false;
    bool changedPosition = false;

    void renderLate(float dt,
                    std::shared_ptr<opal::CommandBuffer> commandBuffer,
                    bool updatePipeline);
    void updateLate(Window &window);
    void setLateViewMatrix(const glm::mat4 &view);
    void setLateProjectionMatrix(const glm::mat4 &projection);
    std::optional<std::shared_ptr<opal::Pipeline>>
    getLateShaderPipelineInternal();
    void setLatePipeline(std::shared_ptr<opal::Pipeline> pipeline);
    bool lateCanCastShadows() const;
};

/**
 * @brief A UIObject is a GameObject that is used for creating user
 * interface elements.
 *
 */
class UIObject : public GameObject {
    bool canUseDeferredRendering() override { return false; }
};

/**
 * @brief A conjunction of UI elements that share the same view and
 * projection matrices. Acts as a container for organizing UI objects.
 *
 */
class UIView : public UIObject {
  public:
    /**
     * @brief Renders the view alongside all registered child UI objects in
     * submission order.
     */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;
    /**
     * @brief Stores the view matrix used when rendering the UI hierarchy.
     */
    void setViewMatrix(const glm::mat4 &view) override;
    /**
     * @brief Stores the projection matrix applied to all child UI objects.
     */
    void setProjectionMatrix(const glm::mat4 &projection) override;

    /**
     * @brief Adds a child UI object to this view.
     *
     * @param child The UI object to add. \warning The object must be
     * long-lived.
     */
    inline void addChild(UIObject *child) { children.push_back(child); }

  private:
    std::vector<UIObject *> children;
};

#endif // ATLAS_COMPONENT_H
