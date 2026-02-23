/*
 body.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Body definitions
 Copyright (c) 2025 maxvdec
*/

/// \ingroup library

#ifndef BEZEL_BODY_H
#define BEZEL_BODY_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include "atlas/units.h"

class Window;

/**
 * @brief Structure representing a point of intersection in both world and model
 * space.
 *
 */
struct IntersectionPoint {
    glm::vec3 worldSpacePoint;
    glm::vec3 modelSpacePoint;
};

class Body;

/**
 * @brief Structure representing a contact between two physics bodies.
 *
 */
struct Contact {
    IntersectionPoint pointA;
    IntersectionPoint pointB;

    glm::vec3 normal;
    float separationDistance;
    float timeOfImpact;

    std::shared_ptr<Body> bodyA;
    std::shared_ptr<Body> bodyB;

    /**
     * @brief Compares this contact with another for sorting purposes.
     *
     * @param other The other contact to compare with.
     * @return (int) Comparison result.
     */
    int compareTo(const Contact other) const;
};

/**
 * @brief Class representing a physical body in a physics simulation. It
 * includes methods for applying forces, detecting collisions, and updating the
 * body's state.
 * \subsection body-example Example
 * ```cpp
 * // Create a physics body
 * auto body = std::make_shared<Body>();
 * // Set the body's position
 * body->position = {0.0, 0.0, 0.0};
 * // Apply a mass of 2.0 units
 * body->applyMass(2.0f);
 * // Set the body's shape to a sphere with radius 1.0
 * body->shape = std::make_shared<Sphere>(1.0f);
 * // Set the body's elasticity and friction
 * body->elasticity = 0.8f;
 * body->friction = 0.5f;
 * ```
 */
class Body {
  public:
    Position3d position;
    glm::quat orientation;
    std::shared_ptr<Shape> shape;
    glm::vec3 linearVelocity = {0.0f, 0.0f, 0.0f};
    glm::vec3 angularVelocity = {0.0f, 0.0f, 0.0f};
    float invMass = 0.0f;
    float elasticity = 0.0f;
    float friction = 0.5f;

    /**
     * @brief Gets the center of mass in world space coordinates.
     *
     * @return (glm::vec3) World space center of mass.
     */
    glm::vec3 getCenterOfMassWorldSpace() const;
    /**
     * @brief Gets the center of mass in model space coordinates.
     *
     * @return (glm::vec3) Model space center of mass.
     */
    glm::vec3 getCenterOfMassModelSpace() const;

    /**
     * @brief Converts a point from world space to model space.
     *
     * @param point The point in world space.
     * @return (glm::vec3) The point in model space.
     */
    glm::vec3 worldSpaceToModelSpace(const glm::vec3 &point) const;
    /**
     * @brief Converts a point from model space to world space.
     *
     * @param point The point in model space.
     * @return (glm::vec3) The point in world space.
     */
    glm::vec3 modelSpaceToWorldSpace(const glm::vec3 &point) const;

    /**
     * @brief Gets the inverse inertia tensor in world space.
     *
     * @return (glm::mat3) Inverse inertia tensor in world space.
     */
    glm::mat3 getInverseInertiaTensorWorldSpace() const;
    /**
     * @brief Gets the inverse inertia tensor in body space.
     *
     * @return (glm::mat3) Inverse inertia tensor in body space.
     */
    glm::mat3 getInverseInertiaTensorBodySpace() const;

    /**
     * @brief Applies an impulse at a specific point on the body.
     *
     * @param point The point where the impulse is applied.
     * @param impulse The impulse vector to apply.
     */
    void applyImpulse(const glm::vec3 &point, const glm::vec3 &impulse);
    /**
     * @brief Applies a linear impulse to the body's center of mass.
     *
     * @param impulse The linear impulse vector to apply.
     */
    void applyLinearImpulse(const glm::vec3 &impulse);
    /**
     * @brief Applies an angular impulse to the body.
     *
     * @param impulse The angular impulse vector to apply.
     */
    void applyAngularImpulse(const glm::vec3 &impulse);

    /**
     * @brief Applies mass to the body, setting the inverse mass.
     *
     * @param mass The mass to apply. If <= 0, creates infinite mass.
     */
    inline void applyMass(float mass) {
        if (mass <= 0.0f) {
            invMass = 0.0f; // Infinite mass
        } else {
            invMass = 1.0f / mass;
        }
    }

    /**
     * @brief Gets the mass of the body.
     *
     * @return (float) The mass of the body, or INFINITY for infinite mass.
     */
    inline float getMass() const {
        if (invMass == 0.0f) {
            return INFINITY;
        }
        return 1.0f / invMass;
    }

    /**
     * @brief Updates the body within the context of a window.
     *
     * @param window The window context for the update.
     */
    void update(Window &window);

    /**
     * @brief Tests static intersection between two bodies.
     *
     * @param bodyA The first body.
     * @param bodyB The second body.
     * @param contact Output contact information.
     * @return (bool) True if intersection detected, false otherwise.
     */
    static bool intersectsStatic(const std::shared_ptr<Body> &bodyA,
                                 const std::shared_ptr<Body> &bodyB,
                                 Contact &contact);
    /**
     * @brief Tests intersection between two bodies with time step.
     *
     * @param bodyA The first body.
     * @param bodyB The second body.
     * @param contact Output contact information.
     * @param dt Delta time for the test.
     * @return (bool) True if intersection detected, false otherwise.
     */
    static bool intersects(const std::shared_ptr<Body> &bodyA,
                           const std::shared_ptr<Body> &bodyB, Contact &contact,
                           float dt);

    /**
     * @brief Resolves a contact between bodies.
     *
     * @param contact The contact to resolve.
     */
    void resolveContact(Contact &contact);
    /**
     * @brief Updates the physics simulation for this body.
     *
     * @param dt Delta time for the physics step.
     */
    void updatePhysics(double dt);

  private:
    std::shared_ptr<Body> thisShared = nullptr;

    bool isSleeping = false;
    float sleepTimer = 0.0f;

    static constexpr float SLEEP_TIME_THRESHOLD = 0.5f;
    static constexpr float SLEEP_LINEAR_THRESHOLD = 0.05f;
    static constexpr float SLEEP_ANGULAR_THRESHOLD = 0.1f;
};

/**
 * @brief Structure representing a pseudo body for collision detection
 * algorithms.
 *
 */
struct PseudoBody {
    int id;
    float value;
    bool ismin;
};

/**
 * @brief Namespace containing physics utility functions.
 *
 */
namespace bezel {
/**
 * @brief Computes support point for collision detection between two bodies.
 *
 * @param bodyA The first body.
 * @param bodyB The second body.
 * @param dir Direction vector for support calculation.
 * @param bias Bias value for the calculation.
 * @return (Point) The computed support point.
 */
Point support(const std::shared_ptr<Body> bodyA,
              const std::shared_ptr<Body> bodyB, glm::vec3 dir, float bias);
} // namespace bezel

#endif // BEZEL_BODY_H
