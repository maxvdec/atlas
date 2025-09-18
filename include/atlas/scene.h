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

class Window;

class Scene {
  public:
    virtual void update(Window &window) = 0;
    virtual void initialize(Window &window) = 0;
};

#endif // ATLAS_SCENE_H
