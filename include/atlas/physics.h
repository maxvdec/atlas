//
// physics.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The physics components for Atlas
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_PHYSICS_H
#define ATLAS_PHYSICS_H

#include "atlas/units.h"
#include <bezel/bezel.h>
#include <atlas/component.h>
#include <memory>
#include <string>
#include <vector>

using MotionType = bezel::MotionType;

struct RaycastHit {
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    float distance = 0.0;
    GameObject *object = nullptr;
    bezel::Rigidbody *rigidbody = nullptr;
    bool didHit = false;
};

struct RaycastResult {
    std::vector<RaycastHit> hits;
    RaycastHit hit;
    float closestDistance = 0.0f;
};

enum class QueryOperation {
    RaycastAll,
    Raycast,
    RaycastWorld,
    RaycastWorldAll,
    RaycastTagged,
    RaycastTaggedAll,
};

struct QueryResult {
    QueryOperation operation = QueryOperation::Raycast;
    RaycastResult raycastResult;
};

class Rigidbody final : public Component {
  public:
    std::shared_ptr<bezel::Rigidbody> body;

    void atAttach() override;
    void init() override;
    void beforePhysics() override;
    void update(float dt) override;

    void addCapsuleCollider(float radius, float height);
    void addBoxCollider(const Position3d &extents);
    void addSphereCollider(float radius);
    void addMeshCollider();
    void setFriction(float friction);

    void applyForce(const Position3d &force);
    void applyForceAtPoint(const Position3d &force, const Position3d &point);
    void applyImpulse(const Position3d &impulse);
    void setLinearVelocity(const Position3d &velocity);
    void addLinearVelocity(const Position3d &velocity);
    void setAngularVelocity(const Position3d &velocity);
    void addAngularVelocity(const Position3d &velocity);

    void raycast(const Position3d &direction, float maxDistance = 1000.0f);
    void raycastAll(const Position3d &direction, float maxDistance = 100.0f);
    void raycastWorld(const Position3d &origin, const Position3d &direction,
                      float maxDistance = 1000.0f);
    void raycastWorldAll(const Position3d &origin, const Position3d &direction,
                         float maxDistance = 1000.0f);
    void raycastTagged(const std::vector<std::string> &tags,
                       const Position3d &direction,
                       float maxDistance = 1000.0f);
    void raycastTaggedAll(const std::vector<std::string> &tags,
                          const Position3d &direction,
                          float maxDistance = 1000.0f);

    bool hasTag(const std::string &tag) const;
    void addTag(const std::string &tag);
    void removeTag(const std::string &tag);

    void setDamping(float linearDamping, float angularDamping);

    void setMass(float mass);

    void setRestitution(float restitution);

    void setMotionType(MotionType motionType);
    Rigidbody() = default;
};

#endif // ATLAS_PHYSICS_H