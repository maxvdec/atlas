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

struct OverlapHit {
    Position3d contactPoint = {0.0f, 0.0f, 0.0f};
    Point3d penetrationAxis = {0.0f, 0.0f, 0.0f};
    float penetrationDepth = 0.0f;
    GameObject *object = nullptr;
    bezel::Rigidbody *rigidbody = nullptr;
};

struct OverlapResult {
    std::vector<OverlapHit> hits;
    bool hitAny = false;
};

struct SweepHit {
    Position3d position = {0.0f, 0.0f, 0.0f};
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    float distance = 0.0f;
    float percentage = 0.0f;
    GameObject *object = nullptr;
    bezel::Rigidbody *rigidbody = nullptr;
};

struct SweepResult {
    std::vector<SweepHit> hits;
    SweepHit closest;
    bool hitAny = false;
    Position3d endPosition = {0.0f, 0.0f, 0.0f};
};

enum class QueryOperation {
    RaycastAll,
    Raycast,
    RaycastWorld,
    RaycastWorldAll,
    RaycastTagged,
    RaycastTaggedAll,
    Movement,
    Overlap,
    MovementAll,
};

struct QueryResult {
    QueryOperation operation = QueryOperation::Raycast;
    RaycastResult raycastResult;
    OverlapResult overlapResult;
    SweepResult sweepResult;
};

struct WorldBody {};

using JointChild = std::variant<GameObject *, WorldBody>;

enum class SpringMode { FrequencyAndDamping, StiffnessAndDamping };

enum class Space { Local, Global };

struct Spring {
    bool enabled = false;

    SpringMode mode = SpringMode::FrequencyAndDamping;

    float frequencyHz = 0.0f;
    float dampingRatio = 0.0f;

    float stiffness = 0.0f;
    float damping = 0.0f;
};

struct AngleLimits {
    bool enabled = false;
    float minAngle = 0.0f;
    float maxAngle = 0.0f;
};

struct Motor {
    bool enabled = false;
    float maxForce = 0.0f;
    float maxTorque = 0.0f;
};

class Joint : public Component {
  public:
    virtual ~Joint() = default;

    JointChild parent;
    JointChild child;

    Space space = Space::Global;

    Position3d anchor = Position3d::invalid();

    float breakForce = 0.0f;
    float breakTorque = 0.0f;

    void beforePhysics() override = 0;
};

class FixedJoint final : public Joint {
  private:
    std::shared_ptr<bezel::FixedJoint> joint;

  public:
    void beforePhysics() override;
};

class HingeJoint final : public Joint {
  private:
    std::shared_ptr<bezel::HingeJoint> joint;

  public:
    Normal3d axis1 = Normal3d::up();
    Normal3d axis2 = Normal3d::up();

    AngleLimits limits;
    Motor motor;

    void beforePhysics() override;
};

class SpringJoint final : public Joint {
  private:
    std::shared_ptr<bezel::SpringJoint> joint;

  public:
    Position3d anchorB = Position3d::invalid();

    float restLength = 1.0f;

    bool useLimits = false;
    float minLength = 0.0f;
    float maxLength = 0.0f;

    Spring spring;

    void beforePhysics() override;
};

class Rigidbody : public Component {
  public:
    std::shared_ptr<bezel::Rigidbody> body;
    std::string sendSignal;
    bool isSensor = false;

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

    void overlapCapsule(float radius, float height);
    void overlapBox(const Position3d &extents);
    void overlapSphere(float radius);
    void overlap(); // Using existing collider

    void overlapCapsuleWorld(const Position3d &position, float radius,
                             float height);
    void overlapBoxWorld(const Position3d &position, const Position3d &extents);
    void overlapSphereWorld(const Position3d &position, float radius);

    void predictMovementCapsule(const Position3d &endPosition, float radius,
                                float height);
    void predictMovementBox(const Position3d &endPosition,
                            const Position3d &extents);
    void predictMovementSphere(const Position3d &endPosition, float radius);
    void
    predictMovement(const Position3d &endPosition); // Using existing collider

    void predictMovementCapsuleAll(const Position3d &endPosition, float radius,
                                   float height);
    void predictMovementBoxAll(const Position3d &endPosition,
                               const Position3d &extents);
    void predictMovementSphereAll(const Position3d &endPosition, float radius);
    void predictMovementAll(
        const Position3d &endPosition); // Using existing collider

    void predictMovementCapsuleWorld(const Position3d &startPosition,
                                     const Position3d &endPosition,
                                     float radius, float height);
    void predictMovementBoxWorld(const Position3d &startPosition,
                                 const Position3d &endPosition,
                                 const Position3d &extents);
    void predictMovementSphereWorld(const Position3d &startPosition,
                                    const Position3d &endPosition,
                                    float radius);

    void predictMovementCapsuleWorldAll(const Position3d &startPosition,
                                        const Position3d &endPosition,
                                        float radius, float height);
    void predictMovementBoxWorldAll(const Position3d &startPosition,
                                    const Position3d &endPosition,
                                    const Position3d &extents);
    void predictMovementSphereWorldAll(const Position3d &startPosition,
                                       const Position3d &endPosition,
                                       float radius);

    bool hasTag(const std::string &tag) const;
    void addTag(const std::string &tag);
    void removeTag(const std::string &tag);

    void setDamping(float linearDamping, float angularDamping);

    void setMass(float mass);

    void setRestitution(float restitution);

    void setMotionType(MotionType motionType);
    Rigidbody() = default;
};

class Sensor final : public Rigidbody {
  public:
    Sensor() { Rigidbody::isSensor = true; }

    void setSignal(const std::string &signal) { sendSignal = signal; }
};

#endif // ATLAS_PHYSICS_H