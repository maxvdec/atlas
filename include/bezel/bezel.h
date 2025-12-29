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
#include <string>
#include <vector>
#ifndef BEZEL_NATIVE
#include <bezel/jolt/world.h>
#endif

#ifndef BEZEL_NATIVE
using namespace bezel_jolt;
#endif

class Window;

namespace bezel {

constexpr uint32_t INVALID_JOLT_ID = UINT32_MAX;

struct BodyIdentifier {
    uint32_t joltId;
    uint32_t atlasId;
};

enum class MotionType { Static, Dynamic, Kinematic };

class PhysicsWorld;

class Collider {
  public:
    virtual ~Collider() = default;
    virtual float getMinExtent() const = 0;

#ifndef BEZEL_NATIVE
    virtual JPH::RefConst<JPH::Shape> getJoltShape() const = 0;
#endif
};

class BoxCollider : public Collider {
  public:
    Position3d halfExtents;

    BoxCollider(const Position3d &halfExtents) : halfExtents(halfExtents) {}

    float getMinExtent() const override {
        return std::min({halfExtents.x, halfExtents.y, halfExtents.z});
    }

#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class CapsuleCollider : public Collider {
  public:
    float radius;
    float height;

    float getMinExtent() const override {
        return std::min(radius * 2.0f, height);
    }

    CapsuleCollider(float radius, float height)
        : radius(radius), height(height) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class SphereCollider : public Collider {
  public:
    float radius;

    float getMinExtent() const override { return radius * 2.0f; }

    explicit SphereCollider(float radius) : radius(radius) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class MeshCollider : public Collider {
  public:
    std::vector<Position3d> vertices;
    std::vector<uint32_t> indices;

    float getMinExtent() const override {
        float minExtent = std::numeric_limits<float>::max();
        for (const auto &vert : vertices) {
            minExtent = std::min({minExtent, vert.x, vert.y, vert.z});
        }
        return minExtent;
    }

    MeshCollider(const std::vector<Position3d> &vertices,
                 const std::vector<uint32_t> &indices);
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class CollisionDispatcher {
  public:
    virtual ~CollisionDispatcher() = default;

    virtual void update(bezel::PhysicsWorld *world) = 0;
    virtual void setup(bezel::PhysicsWorld *world) = 0;
};

struct RaycastHit {
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    float distance = 0.0;
    bezel::Rigidbody *rigidbody = nullptr;
    bool didHit = false;
};

struct RaycastResult {
    float closestDistance = -1;
    std::vector<RaycastHit> hits;
    RaycastHit hit;
};

struct OverlapHit {
    Position3d contactPoint = {0.0f, 0.0f, 0.0f};
    Point3d penetrationAxis = {0.0f, 0.0f, 0.0f};
    float penetrationDepth = 0.0f;
    bezel::Rigidbody *rigidbody = nullptr;
};

struct OverlapResult {
    std::vector<OverlapHit> hits;
    bool hitAny = false;
};

struct SweepHit {
    bezel::Rigidbody *rigidbody = nullptr;
    float distance = 0.0f;
    float percentage = 0.0f;
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
};

struct SweepResult {
    std::vector<SweepHit> hits;
    SweepHit closest;
    bool hitAny = false;
};

struct Rigidbody {
    Position3d position;
    Rotation3d rotation;
    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bool isSensor = false;
    std::string sensorSignal;

    float mass = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;

    std::vector<std::string> tags;

    Position3d linearVelocity = {-1.0f, -1.0f, -1.0f};
    Position3d angularVelocity = {-1.0f, -1.0f, -1.0f};
    Position3d impulse = {0.0f, 0.0f, 0.0f};
    Position3d force = {0.0f, 0.0f, 0.0f};
    Position3d forcePoint = {0.0f, 0.0f, 0.0f};

    float linearDamping = 0.05f;
    float angularDamping = 0.1f;

    bool addLinearVelocity = false;
    bool addAngularVelocity = false;

    void setPosition(const Position3d &position,
                     std::shared_ptr<PhysicsWorld> world);
    void setRotation(const Rotation3d &rotation,
                     std::shared_ptr<PhysicsWorld> world);

    RaycastResult raycast(const Position3d &direction, float maxDistance,
                          std::shared_ptr<PhysicsWorld> world,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID);
    RaycastResult raycastAll(const Position3d &direction, float maxDistance,
                             std::shared_ptr<PhysicsWorld> world,
                             uint32_t ignoreBodyId = INVALID_JOLT_ID);

    OverlapResult overlap(std::shared_ptr<PhysicsWorld> world,
                          std::shared_ptr<Collider> collider,
                          const Position3d &position,
                          const Rotation3d &rotation,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID);

    SweepResult sweep(std::shared_ptr<PhysicsWorld> world,
                      std::shared_ptr<Collider> collider,
                      const Position3d &direction, Position3d &endPosition,
                      uint32_t ignoreBodyId = INVALID_JOLT_ID);

    SweepResult sweepAll(std::shared_ptr<PhysicsWorld> world,
                         std::shared_ptr<Collider> collider,
                         const Position3d &direction, Position3d &endPosition,
                         uint32_t ignoreBodyId = INVALID_JOLT_ID);

    std::shared_ptr<Collider> collider;

    BodyIdentifier id = {.joltId = INVALID_JOLT_ID, .atlasId = 0};
    MotionType motionType = MotionType::Dynamic;

    void create(std::shared_ptr<PhysicsWorld> world);
    void setCollider(std::shared_ptr<Collider> collider);

    void applyProperties(std::shared_ptr<PhysicsWorld> world);

    void refresh(std::shared_ptr<PhysicsWorld> world);
    void destroy(std::shared_ptr<PhysicsWorld> world);
};

class PhysicsWorld {
#ifndef BEZEL_NATIVE
    std::unique_ptr<JPH::TempAllocatorMalloc> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;

    BroadPhaseLayerImpl broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl objectLayerPairFilter;
    std::shared_ptr<CollisionDispatcher> collisionDispatcher;

    std::vector<BodyIdentifier> bodies;

#endif
  public:
#ifndef BEZEL_NATIVE

    JPH::PhysicsSystem physicsSystem;
#endif
    bool initialized = false;

    void init();

    void update(float dt);

    RaycastResult raycast(const Position3d &origin, const Position3d &direction,
                          float maxDistance,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID);
    RaycastResult raycastAll(const Position3d &origin,
                             const Position3d &direction, float maxDistance,
                             uint32_t ignoreBodyId = INVALID_JOLT_ID);

    OverlapResult overlap(std::shared_ptr<PhysicsWorld> world,
                          std::shared_ptr<Collider> collider,
                          const Position3d &position,
                          const Rotation3d &rotation,
                          uint32_t ignoreBodyId = INVALID_JOLT_ID);

    SweepResult sweep(std::shared_ptr<PhysicsWorld> world,
                      std::shared_ptr<Collider> collider,
                      const Position3d &startPosition,
                      const Rotation3d &startRotation,
                      const Position3d &direction, Position3d &endPosition,
                      uint32_t ignoreBodyId = INVALID_JOLT_ID);

    SweepResult sweepAll(std::shared_ptr<PhysicsWorld> world,
                         std::shared_ptr<Collider> collider,
                         const Position3d &startPosition,
                         const Rotation3d &startRotation,
                         const Position3d &direction, Position3d &endPosition,
                         uint32_t ignoreBodyId = INVALID_JOLT_ID);

    void addBody(std::shared_ptr<bezel::Rigidbody> body);

    void setGravity(const Position3d &gravity);

    ~PhysicsWorld();
};

} // namespace bezel

#endif