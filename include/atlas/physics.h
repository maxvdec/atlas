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

/**
 * @file atlas/physics.h
 * @brief High-level physics components and query API for Atlas.
 *
 * This header provides engine-facing physics components (e.g. `Rigidbody`,
 * `Vehicle`, joints) and query result containers (raycasts, overlaps, sweeps).
 *
 * \note This is an alpha API and may change.
 *
 * \subsection physics-example Example
 * ```cpp
 * // Attach physics to a GameObject via components
 * auto rb = std::make_shared<Rigidbody>();
 * rb->setMotionType(MotionType::Dynamic);
 * rb->setMass(5.0f);
 * rb->addBoxCollider({0.5f, 0.5f, 0.5f});
 * rb->setFriction(0.8f);
 *
 * // Optional tagging for filtered queries
 * rb->addTag("Player");
 *
 * // Trigger a raycast query (results are delivered via onQueryRecieve)
 * rb->raycast({0.0f, -1.0f, 0.0f}, 100.0f);
 *
 * // For sensors, you can emit signals on contact
 * auto sensor = std::make_shared<Sensor>();
 * sensor->addSphereCollider(1.0f);
 * sensor->setSignal("EnteredTrigger");
 * ```
 */

using MotionType = bezel::MotionType;

/**
 * @brief Single hit returned by a raycast query.
 */
struct RaycastHit {
    /** @brief World-space impact position. */
    Position3d position = {0.0f, 0.0f, 0.0f};
    /** @brief Surface normal at the impact point. */
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    /** @brief Distance from ray origin to the hit point. */
    float distance = 0.0;
    /** @brief Atlas-side object that owns the rigidbody, when known. */
    GameObject *object = nullptr;
    /** @brief Underlying Bezel rigidbody pointer, when known. */
    bezel::Rigidbody *rigidbody = nullptr;
    /** @brief Whether the query produced a valid hit. */
    bool didHit = false;
};

/**
 * @brief Aggregated results for raycast queries.
 */
struct RaycastResult {
    /** @brief All hits (for `RaycastAll` variants). */
    std::vector<RaycastHit> hits;
    /** @brief Primary hit (for `Raycast` variants). */
    RaycastHit hit;
    /** @brief Convenience distance for the nearest impact. */
    float closestDistance = 0.0f;
};

/**
 * @brief Single hit returned by an overlap query.
 */
struct OverlapHit {
    /** @brief World-space contact point. */
    Position3d contactPoint = {0.0f, 0.0f, 0.0f};
    /** @brief Axis along which penetration occurs. */
    Point3d penetrationAxis = {0.0f, 0.0f, 0.0f};
    /** @brief Signed penetration depth (implementation-defined sign). */
    float penetrationDepth = 0.0f;
    /** @brief Atlas-side owner object, when known. */
    GameObject *object = nullptr;
    /** @brief Underlying Bezel rigidbody pointer, when known. */
    bezel::Rigidbody *rigidbody = nullptr;
};

/**
 * @brief Aggregated results for overlap queries.
 */
struct OverlapResult {
    /** @brief All overlap hits. */
    std::vector<OverlapHit> hits;
    /** @brief Whether any overlap occurred. */
    bool hitAny = false;
};

/**
 * @brief Single hit returned by a sweep query.
 */
struct SweepHit {
    /** @brief World-space impact position. */
    Position3d position = {0.0f, 0.0f, 0.0f};
    /** @brief Surface normal at the impact point. */
    Normal3d normal = {0.0f, 0.0f, 0.0f};
    /** @brief Distance traveled before impact. */
    float distance = 0.0f;
    /** @brief Impact fraction along the sweep in [0, 1]. */
    float percentage = 0.0f;
    /** @brief Atlas-side owner object, when known. */
    GameObject *object = nullptr;
    /** @brief Underlying Bezel rigidbody pointer, when known. */
    bezel::Rigidbody *rigidbody = nullptr;
};

/**
 * @brief Aggregated results for sweep (movement prediction) queries.
 */
struct SweepResult {
    /** @brief All sweep hits (for "All" variants). */
    std::vector<SweepHit> hits;
    /** @brief Closest hit for convenience. */
    SweepHit closest;
    /** @brief Whether any hit occurred. */
    bool hitAny = false;
    /** @brief End position of the sweep when no hit blocks the movement. */
    Position3d endPosition = {0.0f, 0.0f, 0.0f};
};

/**
 * @brief High-level query operations issued by `Rigidbody`.
 */
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

/**
 * @brief Union-like container describing the last physics query response.
 */
struct QueryResult {
    /** @brief Which query type produced this result. */
    QueryOperation operation = QueryOperation::Raycast;
    /** @brief Raycast output payload. */
    RaycastResult raycastResult;
    /** @brief Overlap output payload. */
    OverlapResult overlapResult;
    /** @brief Sweep output payload. */
    SweepResult sweepResult;
};

/**
 * @brief Marker type representing the static physics world as a joint target.
 */
struct WorldBody {};

/**
 * @brief Joint endpoint that can reference either a `GameObject` or the world.
 */
using JointChild = std::variant<GameObject *, WorldBody>;

/**
 * @brief Supported spring configuration modes.
 */
enum class SpringMode { FrequencyAndDamping, StiffnessAndDamping };

/**
 * @brief Whether a value is interpreted in local or global space.
 */
enum class Space { Local, Global };

/**
 * @brief Spring tuning parameters.
 */
struct Spring {
    /** @brief Enables the spring contribution. */
    bool enabled = false;

    SpringMode mode = SpringMode::FrequencyAndDamping;

    float frequencyHz = 0.0f;
    float dampingRatio = 0.0f;

    float stiffness = 0.0f;
    float damping = 0.0f;
};

/**
 * @brief Angular constraints for hinge-style joints.
 */
struct AngleLimits {
    bool enabled = false;
    float minAngle = 0.0f;
    float maxAngle = 0.0f;
};

/**
 * @brief Motor settings for joint actuation.
 */
struct Motor {
    bool enabled = false;
    float maxForce = 0.0f;
    float maxTorque = 0.0f;
};

/**
 * @brief Base component for constraint-style joints.
 *
 * Joints are updated during the physics step (`beforePhysics`). The joint
 * endpoints can reference another `GameObject` or the static world.
 */
class Joint : public Component {
  public:
    virtual ~Joint() = default;

    JointChild parent;
    JointChild child;

    Space space = Space::Global;

    Position3d anchor = Position3d::invalid();

    float breakForce = 0.0f;
    float breakTorque = 0.0f;

    /**
     * @brief Called during the physics step to create/update the constraint.
     */
    void beforePhysics() override = 0;

    /**
     * @brief Breaks the joint (implementation-specific).
     */
    virtual void breakJoint() = 0;
};

/**
 * @brief Joint that locks relative translation and rotation.
 */
class FixedJoint final : public Joint {
  private:
    std::shared_ptr<bezel::FixedJoint> joint;

  public:
    /** @brief Synchronizes/creates the native fixed joint. */
    void beforePhysics() override;
    /** @brief Destroys/invalidates the native joint. */
    void breakJoint() override;
};

/**
 * @brief Joint that constrains rotation around a hinge axis.
 */
class HingeJoint final : public Joint {
  private:
    std::shared_ptr<bezel::HingeJoint> joint;

  public:
    Normal3d axis1 = Normal3d::up();
    Normal3d axis2 = Normal3d::up();

    AngleLimits limits;
    Motor motor;

    /** @brief Synchronizes/creates the native hinge joint. */
    void beforePhysics() override;
    /** @brief Destroys/invalidates the native joint. */
    void breakJoint() override;
};

/**
 * @brief Joint that behaves like a distance constraint with optional spring.
 */
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

    /** @brief Synchronizes/creates the native spring joint. */
    void beforePhysics() override;
    /** @brief Destroys/invalidates the native joint. */
    void breakJoint() override;
};

/**
 * @brief Vehicle component backed by Bezel's vehicle implementation.
 */
class Vehicle final : public Component {
  private:
    bezel::Vehicle vehicle;
    bool created = false;

  public:
    bezel::VehicleSettings settings;

    float forward = 0.0f;   // [-1, 1]
    float right = 0.0f;     // [-1, 1]
    float brake = 0.0f;     // [0, 1]
    float handBrake = 0.0f; // [0, 1]

    /** @brief Creates the vehicle constraint when attached. */
    void atAttach() override;
    /** @brief Steps vehicle simulation and forwards driver input. */
    void beforePhysics() override;

    /** @brief Requests a full rebuild of the internal vehicle constraint. */
    void requestRecreate();
};

/**
 * @brief Component that binds a Bezel rigidbody to a `GameObject`.
 *
 * Most query APIs (`raycast*`, `overlap*`, `predictMovement*`) are async-style:
 * the request is queued and the result is reported to components via
 * `Component::onQueryRecieve(QueryResult&)`.
 */
class Rigidbody : public Component {
  public:
    /** @brief Underlying Bezel rigidbody instance. */
    std::shared_ptr<bezel::Rigidbody> body;
    /** @brief Signal string sent by sensors on overlap/contact events. */
    std::string sendSignal;
    /** @brief Whether this rigidbody behaves as a sensor (trigger). */
    bool isSensor = false;

    /** @brief Called when attached to a `GameObject`. */
    void atAttach() override;
    /** @brief Initializes the rigidbody and default collider state. */
    void init() override;
    /** @brief Synchronizes forces and constraints before stepping physics. */
    void beforePhysics() override;
    /** @brief Updates transform synchronization and query dispatch. */
    void update(float dt) override;

    /** @brief Adds a capsule collider to the rigidbody. */
    void addCapsuleCollider(float radius, float height);
    /** @brief Adds a box collider to the rigidbody. */
    void addBoxCollider(const Position3d &extents);
    /** @brief Adds a sphere collider to the rigidbody. */
    void addSphereCollider(float radius);
    /** @brief Adds a mesh collider from the owning object's mesh (if any). */
    void addMeshCollider();
    /** @brief Sets friction coefficient for contact resolution. */
    void setFriction(float friction);

    /** @brief Applies a continuous force in world space. */
    void applyForce(const Position3d &force);
    /** @brief Applies a continuous force at a world-space point. */
    void applyForceAtPoint(const Position3d &force, const Position3d &point);
    /** @brief Applies an instantaneous impulse in world space. */
    void applyImpulse(const Position3d &impulse);
    /** @brief Sets the rigidbody's linear velocity. */
    void setLinearVelocity(const Position3d &velocity);
    /** @brief Adds to the rigidbody's linear velocity. */
    void addLinearVelocity(const Position3d &velocity);
    /** @brief Sets the rigidbody's angular velocity. */
    void setAngularVelocity(const Position3d &velocity);
    /** @brief Adds to the rigidbody's angular velocity. */
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

    /** @brief Returns true if the rigidbody has the provided tag. */
    bool hasTag(const std::string &tag) const;
    /** @brief Adds a tag for filtering and game logic. */
    void addTag(const std::string &tag);
    /** @brief Removes a previously added tag. */
    void removeTag(const std::string &tag);

    /** @brief Sets linear and angular damping coefficients. */
    void setDamping(float linearDamping, float angularDamping);

    /** @brief Sets the mass in kilograms (engine units). */
    void setMass(float mass);

    /** @brief Sets restitution (bounciness) coefficient. */
    void setRestitution(float restitution);

    /** @brief Sets the rigidbody's motion type (static/dynamic/kinematic). */
    void setMotionType(MotionType motionType);
    Rigidbody() = default;
};

/**
 * @brief Convenience sensor rigidbody that defaults `isSensor` to true.
 */
class Sensor final : public Rigidbody {
  public:
    Sensor() { Rigidbody::isSensor = true; }

    /** @brief Sets the signal string emitted when the sensor is triggered. */
    void setSignal(const std::string &signal) { sendSignal = signal; }
};

#endif // ATLAS_PHYSICS_H