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
#include <map>
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
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Core/Reference.h>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

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

namespace bezel {
struct Rigidbody;
}

namespace bezel_jolt {

extern std::map<JPH::BodyID, bezel::Rigidbody *> bodyIdToRigidbodyMap;

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

enum class JoltLogLevel { Info, Warning, Error };

void AtlasLog(JoltLogLevel level, std::string_view msg);
JoltLogLevel Classify(std::string_view s);
void TraceImpl(const char *fmt, ...);
bool AssertFailedImpl(const char *expr, const char *msg, const char *file,
                      JPH::uint line);

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

} // namespace bezel_jolt

#endif
#endif // BEZEL_JOLT_WORLD_H
