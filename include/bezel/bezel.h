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

#ifndef BEZEL_NATIVE
    virtual JPH::RefConst<JPH::Shape> getJoltShape() const = 0;
#endif
};

class BoxCollider : public Collider {
  public:
    Position3d halfExtents;

    BoxCollider(const Position3d &halfExtents) : halfExtents(halfExtents) {}

#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class CapsuleCollider : public Collider {
  public:
    float radius;
    float height;

    CapsuleCollider(float radius, float height)
        : radius(radius), height(height) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class SphereCollider : public Collider {
  public:
    float radius;

    explicit SphereCollider(float radius) : radius(radius) {}
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

class MeshCollider : public Collider {
  public:
    std::vector<Position3d> vertices;
    std::vector<uint32_t> indices;

    MeshCollider(const std::vector<Position3d> &vertices,
                 const std::vector<uint32_t> &indices);
#ifndef BEZEL_NATIVE
    JPH::RefConst<JPH::Shape> getJoltShape() const override;
#endif
};

struct Rigidbody {
    Position3d position;
    Rotation3d rotation;
    glm::quat rotationQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    float mass = 0.0f;
    float friction = 0.5f;
    float restitution = 0.0f;

    Position3d linearVelocity = {-1.0f, -1.0f, -1.0f};
    Position3d angularVelocity = {-1.0f, -1.0f, -1.0f};
    Position3d impulse = {0.0f, 0.0f, 0.0f};
    Position3d force = {0.0f, 0.0f, 0.0f};
    Position3d forcePoint = {0.0f, 0.0f, 0.0f};

    bool addLinearVelocity = false;
    bool addAngularVelocity = false;

    void setPosition(const Position3d &position,
                     std::shared_ptr<PhysicsWorld> world);
    void setRotation(const Rotation3d &rotation,
                     std::shared_ptr<PhysicsWorld> world);

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

    std::vector<BodyIdentifier> bodies;

#endif
  public:
#ifndef BEZEL_NATIVE

    JPH::PhysicsSystem physicsSystem;
#endif
    bool initialized = false;

    void init();

    void update(float dt);

    void addBody(std::shared_ptr<bezel::Rigidbody> body);

    void setGravity(const Position3d &gravity);

    ~PhysicsWorld();
};

} // namespace bezel

#endif
