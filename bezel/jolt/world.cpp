//
// world.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Physics World creation and some Jolt-required classes
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/jolt/world.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/EPhysicsUpdateError.h"
#include "Jolt/RegisterTypes.h"
#include "bezel/bezel.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>

BroadPhaseLayerImpl::BroadPhaseLayerImpl() {
    mObjectToBroadPhase[bezel::jolt::layers::NON_MOVING] =
        bezel::jolt::broad_phase_layers::NON_MOVING;
    mObjectToBroadPhase[bezel::jolt::layers::MOVING] =
        bezel::jolt::broad_phase_layers::MOVING;
    mObjectToBroadPhase[bezel::jolt::layers::SENSOR] =
        bezel::jolt::broad_phase_layers::SENSOR;
}

JPH::uint BroadPhaseLayerImpl::GetNumBroadPhaseLayers() const {
    return bezel::jolt::broad_phase_layers::NUM_BROADPHASE_LAYERS;
}

JPH::BroadPhaseLayer
BroadPhaseLayerImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
    return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char *BroadPhaseLayerImpl::GetBroadPhaseLayerName(
    JPH::BroadPhaseLayer inLayer) const {
    switch (static_cast<JPH::uint>(inLayer.GetValue())) {
    case 0:
        return "NON_MOVING";
    case 1:
        return "MOVING";
    case 2:
        return "SENSOR";
    default:
        return "UNKNOWN";
    }
}
#endif

bool ObjectLayerPairFilterImpl::ShouldCollide(
    JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
    using namespace bezel::jolt::layers;

    if (inObject1 == SENSOR)
        return inObject2 != SENSOR;
    if (inObject2 == SENSOR)
        return inObject1 != SENSOR;

    if (inObject1 == NON_MOVING && inObject2 == NON_MOVING)
        return false;

    return true;
}

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(
    JPH::ObjectLayer inObjectLayer,
    JPH::BroadPhaseLayer inBroadPhaseLayer) const {
    using namespace bezel::jolt::layers;

    switch (inObjectLayer) {
    case NON_MOVING:
        return inBroadPhaseLayer == bezel::jolt::broad_phase_layers::MOVING ||
               inBroadPhaseLayer == bezel::jolt::broad_phase_layers::SENSOR;

    case MOVING:
        return true;

    case SENSOR:
        return inBroadPhaseLayer == bezel::jolt::broad_phase_layers::MOVING ||
               inBroadPhaseLayer == bezel::jolt::broad_phase_layers::NON_MOVING;

    default:
        return false;
    }
}

void bezel::PhysicsWorld::addBody(std::shared_ptr<bezel::Rigidbody> body) {
    bodies.push_back(body->id);
}

void bezel::PhysicsWorld::setGravity(const Position3d &gravity) {
    physicsSystem.SetGravity(JPH::Vec3(static_cast<float>(gravity.x),
                                       static_cast<float>(gravity.y),
                                       static_cast<float>(gravity.z)));
}

void bezel::PhysicsWorld::init() {
    JPH::RegisterDefaultAllocator();

    // Diable tracer for now
    if (JPH::Factory::sInstance == nullptr) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    JPH::RegisterTypes();

    initialized = true;

    constexpr size_t TEMP_ALLOC_SIZE = 32 * 1024 * 1024; // 32 MB
    tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(TEMP_ALLOC_SIZE);

    uint32_t hw = std::max(1u, std::thread::hardware_concurrency());
    uint32_t num_worker_threads = std::max(1u, hw > 1 ? (hw - 1) : 1);

    constexpr uint32_t MAX_JOBS = 4096;
    constexpr uint32_t MAX_BARRIERS = 64;

    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        MAX_JOBS, MAX_BARRIERS, num_worker_threads);

    constexpr uint32_t MAX_BODIES = 65536;
    constexpr uint32_t NUM_BODY_MUTEXES = 0;
    constexpr uint32_t MAX_BODY_PAIRS = 65536;
    constexpr uint32_t MAX_CONTACT_CONSTRAINTS = 65536;

    physicsSystem.Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS,
                       MAX_CONTACT_CONSTRAINTS, broadPhaseLayerInterface,
                       objectVsBroadPhaseLayerFilter, objectLayerPairFilter);

    physicsSystem.SetGravity(JPH::Vec3(0, -9.81f, 0));
}

void bezel::PhysicsWorld::update(float dt) {
    constexpr int COLLISION_STEPS = 1;
    JPH::EPhysicsUpdateError error = physicsSystem.Update(
        dt, COLLISION_STEPS, tempAllocator.get(), jobSystem.get());

    if (error != JPH::EPhysicsUpdateError::None) {
        throw std::runtime_error("Jolt PhysicsWorld update error");
    }
}

bezel::PhysicsWorld::~PhysicsWorld() {
    jobSystem.reset();
    tempAllocator.reset();

    if (initialized) {
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        initialized = false;
    }
}