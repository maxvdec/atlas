/*
 scene.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Scene functions and utilities
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_SCENE_HPP
#define ATLAS_SCENE_HPP

#include "atlas/core/rendering.hpp"
#include "atlas/input.hpp"
#include "atlas/light.hpp"
#include "atlas/texture.hpp"
#include <optional>
#include <vector>

struct Skybox {
    std::optional<Cubemap> cubemap = std::nullopt;
    std::optional<CoreObject> object = std::nullopt;
    RenderingFn dispatcher;

    void addCubemap(Cubemap cubemap);

    Skybox() = default;

    void useSkybox();
};

class Scene : public Interactive {
  public:
    virtual void init() {};
    virtual void update(float deltaTime) {};
    inline void atEachFrame(float deltaTime) override {
        this->update(deltaTime);
    }

    inline void useLight(Light *light) {
        if (light == nullptr) {
            throw std::invalid_argument("Light cannot be null");
        }
        if (this->lights.size() == 10) {
            throw std::runtime_error("Maximum number of lights reached");
        }
        this->lights.push_back(light);
    }
    std::vector<Light *> lights;
    Skybox *skybox;
};

#endif // ATLAS_SCENE_HPP
