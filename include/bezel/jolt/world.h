//
// world.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Physics World implementation settings
// Copyright (c) 2025 maxvdec
//

#ifndef BEZEL_JOLT_WORLD_H
#define BEZEL_JOLT_WORLD_H
#ifndef BEZEL_NATIVE

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/FixedSizeFreeList.h>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include <Jolt/RegisterTypes.h>

namespace bezel::jolt::layers {

static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer SENSOR = 2;

static constexpr JPH::uint NUM_LAYERS = 3;

} // namespace bezel::jolt::layers

namespace bezel::jolt::broad_phase_layers {

static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr JPH::BroadPhaseLayer SENSOR(2);

static constexpr JPH::uint NUM_BROADPHASE_LAYERS = 3;

} // namespace bezel::jolt::broad_phase_layers

class BroadPhaseLayerImpl final : public JPH::BroadPhaseLayerInterface {
  public:
    BroadPhaseLayerImpl();

    JPH::uint GetNumBroadPhaseLayers() const override;
    JPH::BroadPhaseLayer
    GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *
    GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif

  private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[bezel::jolt::layers::NUM_LAYERS];
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
  public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1,
                       JPH::ObjectLayer inLayer2) const override;
};

class ObjectVsBroadPhaseLayerFilterImpl final
    : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1,
                       JPH::BroadPhaseLayer inLayer2) const override;
};

class BodyActivationListenerMain final : public JPH::BodyActivationListener {
  public:
    void OnBodyActivated(const JPH::BodyID &inBodyID,
                         JPH::uint64 inBodyUserData) override {
        (void)inBodyID;
        (void)inBodyUserData;
    };
    void OnBodyDeactivated(const JPH::BodyID &inBodyID,
                           JPH::uint64 inBodyUserData) override {
        (void)inBodyID;
        (void)inBodyUserData;
    };
};

class ContactListenerImpl final : public JPH::ContactListener {
  public:
    JPH::ValidateResult
    OnContactValidate(const JPH::Body &, const JPH::Body &, JPH::RVec3Arg,
                      const JPH::CollideShapeResult &) override {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &, const JPH::Body &,
                        const JPH::ContactManifold &,
                        JPH::ContactSettings &) override {}

    void OnContactPersisted(const JPH::Body &, const JPH::Body &,
                            const JPH::ContactManifold &,
                            JPH::ContactSettings &) override {}

    void OnContactRemoved(const JPH::SubShapeIDPair &) override {}
};

#endif
#endif // BEZEL_JOLT_WORLD_H
