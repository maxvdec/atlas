/*
 scene.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Scene functions
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_SCENE_H
#define ATLAS_SCENE_H

#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/units.h"
#include <memory>
#include <vector>

class Window;

class Scene {
  public:
    virtual void update(Window &window) = 0;
    virtual void initialize(Window &window) = 0;
    virtual void onMouseMove(Window &window, Movement2d movement) {}
    virtual void onMouseScroll(Window &window, Movement2d offset) {}

    AmbientLight ambientLight = {{1.0f, 1.0f, 1.0f}, 0.1f};

    inline void addLight(Light *light) { lights.push_back(light); }

  private:
    std::vector<Light *> lights;

    friend class CoreObject;
};

#endif // ATLAS_SCENE_H
