//
// bezel.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Common Bezel API definition
// Copyright (c) 2025 maxvdec
//

#ifndef BEZEL_H
#define BEZEL_H

#include "atlas/units.h"
#include <memory>
#ifndef BEZEL_NATIVE
#include <bezel/jolt/world.h>
#endif

class Window;

namespace bezel {

class PhysicsWorld {
#ifndef BEZEL_NATIVE
    JPH::PhysicsSystem physicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;

    BroadPhaseLayerImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;

#endif
  public:
    bool initialized = false;

    void init();

    void update(float dt);

    ~PhysicsWorld();
};

} // namespace bezel

#endif
