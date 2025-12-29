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
#include "Jolt/Core/IssueReporting.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/EPhysicsUpdateError.h"
#include "Jolt/RegisterTypes.h"
#include "bezel/bezel.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include "atlas/tracer/log.h"
#include "bezel/jolt/query.h"

namespace bezel_jolt {
std::map<JPH::BodyID, bezel::Rigidbody *> bodyIdToRigidbodyMap;
}

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

void bezel_jolt::AtlasLog(JoltLogLevel level, std::string_view msg) {
    switch (level) {
    case JoltLogLevel::Info:
        atlas_log(std::string("[Jolt] ") + std::string(msg));
        break;
    case JoltLogLevel::Warning:
        atlas_warning(std::string("[Jolt] ") + std::string(msg));
        break;
    case JoltLogLevel::Error:
        atlas_error(std::string("[Jolt] ") + std::string(msg));
        break;
    }
}

JoltLogLevel bezel_jolt::Classify(std::string_view s) {
    if (s.starts_with("Error:") || s.starts_with("ERROR:") ||
        s.starts_with("FATAL") || s.find("failed") != std::string_view::npos ||
        s.find("Out of memory") != std::string_view::npos) {
        return JoltLogLevel::Error;
    }

    if (s.starts_with("Warning:") || s.starts_with("WARN:") ||
        s.find("deprecated") != std::string_view::npos) {
        return JoltLogLevel::Warning;
    }

    return JoltLogLevel::Info;
}

void bezel_jolt::TraceImpl(const char *fmt, ...) {
    char buf[4096];

    va_list args;
    va_start(args, fmt);
    int _ = std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::string_view msg(buf);
    bezel_jolt::AtlasLog(bezel_jolt::Classify(msg), msg);
}

bool bezel_jolt::AssertFailedImpl(const char *expr, const char *msg,
                                  const char *file, JPH::uint line) {
    char buf[4096];
    int _ =
        std::snprintf(buf, sizeof(buf), "Assert failed: %s%s%s (%s:%u)", expr,
                      msg ? " : " : "", msg ? msg : "", file, (unsigned)line);

    bezel_jolt::AtlasLog(JoltLogLevel::Error, buf);

    return true;
}

void bezel::PhysicsWorld::init() {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = bezel_jolt::TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = bezel_jolt::AssertFailedImpl);

    if (JPH::Factory::sInstance == nullptr) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    JPH::RegisterTypes();

    initialized = true;

    tempAllocator = std::make_unique<JPH::TempAllocatorMalloc>();

    uint32_t hw = std::max(1u, std::thread::hardware_concurrency());
    uint32_t num_worker_threads = std::max(1u, hw > 1 ? (hw - 1) : 1);

    constexpr uint32_t MAX_JOBS = 4096;
    constexpr uint32_t MAX_BARRIERS = 64;

    jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        MAX_JOBS, MAX_BARRIERS, num_worker_threads);

    constexpr uint32_t MAX_BODIES = 65536;
    constexpr uint32_t NUM_BODY_MUTEXES = 1024;
    constexpr uint32_t MAX_BODY_PAIRS = 65536;
    constexpr uint32_t MAX_CONTACT_CONSTRAINTS = 65536;

    physicsSystem.Init(MAX_BODIES, NUM_BODY_MUTEXES, MAX_BODY_PAIRS,
                       MAX_CONTACT_CONSTRAINTS, broadPhaseLayerInterface,
                       objectVsBroadPhaseLayerFilter, objectLayerPairFilter);

    physicsSystem.SetGravity(JPH::Vec3(0, -9.81f, 0));

    this->collisionDispatcher = std::make_shared<JoltCollisionDispatcher>();

    collisionDispatcher->setup(this);
}

void bezel::PhysicsWorld::update(float dt) {
    constexpr int COLLISION_STEPS = 1;
    JPH::EPhysicsUpdateError error = physicsSystem.Update(
        dt, COLLISION_STEPS, tempAllocator.get(), jobSystem.get());

    if (error != JPH::EPhysicsUpdateError::None) {
        throw std::runtime_error("Jolt PhysicsWorld update error");
    }

    collisionDispatcher->update(this);
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

bezel::RaycastResult bezel::PhysicsWorld::raycast(const Position3d &origin,
                                                  const Position3d &direction,
                                                  float maxDistance) {
    RaycastResult out;

    JPH::RVec3Arg originJolt(origin.x, origin.y, origin.z);
    JPH::RVec3Arg directionJolt(direction.x, direction.y, direction.z);

    const JPH::RRayCast ray(originJolt,
                            directionJolt.Normalized() * maxDistance);

    JPH::RayCastResult hit;

    const bool didHit = physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit);

    if (!didHit) {
        out.hit.didHit = false;
        return out;
    }

    const float dist = hit.mFraction * maxDistance;
    out.closestDistance = dist;

    RaycastHit h;
    h.didHit = true;
    h.distance = dist;
    h.position =
        Position3d(origin.x + (direction.x * hit.mFraction * maxDistance),
                   origin.y + (direction.y * hit.mFraction * maxDistance),
                   origin.z + (direction.z * hit.mFraction * maxDistance));

    JPH::BodyID bodyId = hit.mBodyID;
    auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
    h.rigidbody =
        (it != bezel_jolt::bodyIdToRigidbodyMap.end()) ? it->second : nullptr;
    JPH::Vec3 normal = -ray.mDirection * hit.mFraction;
    h.normal = Normal3d(normal.GetX(), normal.GetY(), normal.GetZ());

    out.hits.push_back(h);
    out.hit = h;

    return out;
}

bezel::RaycastResult bezel::PhysicsWorld::raycastAll(
    const Position3d &origin, const Position3d &direction, float maxDistance) {
    RaycastResult out;

    JPH::RVec3Arg originJolt(origin.x, origin.y, origin.z);
    JPH::RVec3Arg directionJolt(direction.x, direction.y, direction.z);

    const JPH::RRayCast ray(originJolt,
                            directionJolt.Normalized() * maxDistance);

    class AllHitsCollector : public JPH::CastRayCollector {
      public:
        std::vector<JPH::RayCastResult> hits;

        void AddHit(const JPH::RayCastResult &result) override {
            hits.push_back(result);
        }
    };

    AllHitsCollector collector;
    JPH::RayCastSettings settings;
    physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector);

    if (collector.hits.empty()) {
        out.hit.didHit = false;
        return out;
    }

    float closestDist = std::numeric_limits<float>::max();

    for (const auto &hit : collector.hits) {
        const float dist = hit.mFraction * maxDistance;

        RaycastHit h;
        h.didHit = true;
        h.distance = dist;
        h.position =
            Position3d(origin.x + (direction.x * hit.mFraction * maxDistance),
                       origin.y + (direction.y * hit.mFraction * maxDistance),
                       origin.z + (direction.z * hit.mFraction * maxDistance));

        JPH::BodyID bodyId = hit.mBodyID;
        auto it = bezel_jolt::bodyIdToRigidbodyMap.find(bodyId);
        h.rigidbody = (it != bezel_jolt::bodyIdToRigidbodyMap.end())
                          ? it->second
                          : nullptr;

        JPH::Vec3 normal = -ray.mDirection * hit.mFraction;
        h.normal = Normal3d(normal.GetX(), normal.GetY(), normal.GetZ());

        out.hits.push_back(h);

        if (dist < closestDist) {
            closestDist = dist;
            out.hit = h;
            out.closestDistance = dist;
        }
    }

    return out;
}
