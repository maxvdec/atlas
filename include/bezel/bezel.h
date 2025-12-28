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
#include <cstdint>
#include <memory>
#include <vector>
#ifndef BEZEL_NATIVE
#include <bezel/jolt/world.h>
#endif

class Window;

namespace bezel {

struct BodyIdentifier {
    uint32_t joltId;
    uint32_t atlasId;
};

struct Rigidbody {
    BodyIdentifier id;
};

class PhysicsWorld {
#ifndef BEZEL_NATIVE
    JPH::PhysicsSystem physicsSystem;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;

    BroadPhaseLayerImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;

    std::vector<BodyIdentifier> bodies;

#endif
  public:
    bool initialized = false;

    void init();

    void update(float dt);

    void addBody(std::shared_ptr<bezel::Rigidbody> body);

    ~PhysicsWorld();
};

} // namespace bezel

#endif
