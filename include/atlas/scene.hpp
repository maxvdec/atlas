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

#include "atlas/input.hpp"

class Scene : public Interactive {
  public:
    virtual void init() {};
    virtual void update(float deltaTime) {};
    inline void atEachFrame(float deltaTime) override {
        this->update(deltaTime);
    }
};

#endif // ATLAS_SCENE_HPP
