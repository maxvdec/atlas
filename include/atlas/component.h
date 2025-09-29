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

class Component {
  public:
    virtual void init() {}
    virtual void update(float deltaTime) {}
    virtual void setViewMatrix(const glm::mat4 &view) {}
    virtual void setProjectionMatrix(const glm::mat4 &projection) {}
    Window *getWindow();

    Component() = default;

    GameObject *object = nullptr;
    Body *body = nullptr;
};

template <typename T>
    requires std::is_base_of_v<GameObject, T>
class TraitComponent : public Component {
  public:
    virtual void init() override {};
    void update(float deltaTime) override {
        if (typedObject != nullptr) {
            updateComponent(typedObject);
        }
    }
    virtual void updateComponent(T *object) {}
    inline void setTypedObject(T *obj) { typedObject = obj; }

  private:
    T *typedObject = nullptr;
};

class GameObject : public Renderable {
  public:
    virtual void attachProgram(const ShaderProgram &program) {};
    virtual void createAndAttachProgram(VertexShader &vertexShader,
                                        FragmentShader &fragmentShader) {};
    virtual void attachTexture(const Texture &texture) {};
    virtual void setColor(const Color &color) {};
    virtual void setPosition(const Position3d &newPosition) {};
    virtual void move(const Position3d &deltaPosition) {};
    virtual void setRotation(const Rotation3d &newRotation) {};
    virtual void lookAt(const Position3d &target, const Normal3d &up) {};
    virtual void rotate(const Rotation3d &deltaRotation) {};
    virtual void setScale(const Scale3d &newScale) {};
    virtual void hide() {};
    virtual void show() {};
    virtual void setupPhysics(Body body) {};

    template <typename T>
        requires std::is_base_of_v<Component, T>
    void addComponent(T &&existing) {
        std::shared_ptr<T> component =
            std::make_shared<T>(std::forward<T>(existing));
        component->object = this;
        component->body = this->body.get();
        components.push_back(component);
    }

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

    std::shared_ptr<Body> body = nullptr;

  protected:
    std::vector<std::shared_ptr<Component>> components;
};

class CompoundObject : public GameObject {
  public:
    std::vector<CoreObject *> objects;
    virtual void initialize() override;
    virtual void update(Window &window) override;
    virtual void updateObjects(Window &window) {};
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

    inline void addObject(CoreObject *obj) { objects.push_back(obj); }
};

#endif // ATLAS_COMPONENT_H