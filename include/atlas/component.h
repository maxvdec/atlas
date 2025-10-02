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
#include "bezel/body.h"
#include <memory>
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
    /**
     * @brief Initializes the component. This method is called once when the
     * component is added to a GameObject.
     *
     */
    virtual void init() {}
    /**
     * @brief Updates the component each frame. This method is called every
     * frame before rendering.
     *
     * @param deltaTime The time elapsed since the last frame.
     */
    virtual void update(float deltaTime) {}
    /**
     * @brief Performs changes when the GameObject's view matrix is updated.
     *
     * @param view The new view matrix.
     */
    virtual void setViewMatrix(const glm::mat4 &view) {}
    /**
     * @brief Performs changes when the GameObject's projection matrix is
     * updated.
     *
     * @param projection The new projection matrix.
     */
    virtual void setProjectionMatrix(const glm::mat4 &projection) {}
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
    /**
     * @brief Gets the Body associated with the component.
     *
     */
    Body *body = nullptr;
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
    void update(float deltaTime) override {
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
    virtual void updateComponent(T *object) {}
    /**
     * @brief Sets the typed object reference for the component.
     *
     * @param obj A pointer to the typed GameObject.
     */
    inline void setTypedObject(T *obj) { typedObject = obj; }

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
     * @brief Attaches a shader program to the object.
     *
     * @param program
     */
    virtual void attachProgram(const ShaderProgram &program) {};
    /**
     * @brief Creates and attaches a shader program to the object.
     *
     * @param vertexShader The vertex shader for the program
     * @param fragmentShader The fragment shader for the program
     */
    virtual void createAndAttachProgram(VertexShader &vertexShader,
                                        FragmentShader &fragmentShader) {};
    /**
     * @brief Attaches a texture to the object.
     *
     * @param texture The texture to attach.
     */
    virtual void attachTexture(const Texture &texture) {};
    /**
     * @brief Sets the color of the object.
     *
     * @param color The new color to set.
     */
    virtual void setColor(const Color &color) {};
    /**
     * @brief Sets the position of the object.
     *
     * @param newPosition The new position to set.
     */
    virtual void setPosition(const Position3d &newPosition) {};
    /**
     * @brief Moves the object by a certain amount.
     *
     * @param deltaPosition The amount to move the object by.
     */
    virtual void move(const Position3d &deltaPosition) {};
    /**
     * @brief Sets the rotation of the object.
     *
     * @param newRotation The new rotation to set.
     */
    virtual void setRotation(const Rotation3d &newRotation) {};
    /**
     * @brief Sets the object to look at a specific point in 3D space.
     *
     * @param target The point to look at.
     * @param up The up vector to define the orientation.
     */
    virtual void lookAt(const Position3d &target, const Normal3d &up) {};
    /**
     * @brief Rotates the object by a certain amount.
     *
     * @param deltaRotation The amount to rotate the object by.
     */
    virtual void rotate(const Rotation3d &deltaRotation) {};
    /**
     * @brief Sets the scale of the object.
     *
     * @param newScale The new scale of the object.
     */
    virtual void setScale(const Scale3d &newScale) {};
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
     * @brief Sets up the physics body for the object.
     *
     * @param body The physics body to associate with the object.
     */
    virtual void setupPhysics(Body body) {};

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
        component->body = this->body.get();
        components.push_back(component);
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
     * @brief Gets the physics body associated with the object.
     *
     * @return A shared pointer to the physics body, or nullptr if not set.
     */
    std::shared_ptr<Body> body = nullptr;

  protected:
    std::vector<std::shared_ptr<Component>> components;
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
    std::vector<CoreObject *> objects;

    virtual void initialize() override;

    virtual void update(Window &window) override;
    /**
     * @brief Updates the objects within the compound object.
     *
     * @param window The window where the objects are being rendered.
     */
    virtual void updateObjects(Window &window) {};
    /**
     * @brief Initializes the compound object.
     *
     */
    virtual void init() {};

    void render(float dt) override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;
    std::optional<ShaderProgram> getShaderProgram() override;
    void setShader(const ShaderProgram &shader) override;
    Position3d getPosition() const override;
    std::vector<CoreVertex> getVertices() const override;
    Size3d getScale() const override;
    bool canCastShadows() const override;
    void setPosition(const Position3d &newPosition) override;
    void move(const Position3d &deltaPosition) override;
    void setRotation(const Rotation3d &newRotation) override;
    void lookAt(const Position3d &target, const Normal3d &up) override;
    void rotate(const Rotation3d &deltaRotation) override;
    void setScale(const Scale3d &newScale) override;
    void hide() override;
    void show() override;
    void setupPhysics(Body body) override;

    /**
     * @brief Adds a component to the object.
     *
     * @param obj The component instance to add. \warning It must be long-lived.
     * This means that declaring it as a class property is a good idea.
     */
    inline void addObject(CoreObject *obj) { objects.push_back(obj); }
};

#endif // ATLAS_COMPONENT_H