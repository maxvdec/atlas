//
// joints.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Joint functions for Jolt Backend
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/units.h"
#include <memory>
#include <variant>
#ifndef BEZEL_NATIVE
#include "bezel/bezel.h"
#include "bezel/jolt/world.h"
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include "atlas/tracer/log.h"

void bezel::FixedJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::FixedConstraintSettings settings;

    if (anchor == Position3d::invalid()) {
        settings.mAutoDetectPoint = true;
    } else {
        settings.mPoint1 = JPH::Vec3(anchor.x, anchor.y, anchor.z);
    }

    switch (space) {
    case Space::Global:
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        break;
    case Space::Local:
        settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
        break;
    }

    bool body1IsWorld = false;
    bool body2IsWorld = false;

    if (std::holds_alternative<WorldBody>(parent)) {
        body1IsWorld = true;
    }
    if (std::holds_alternative<WorldBody>(child)) {
        body2IsWorld = true;
    }

    if (body1IsWorld && body2IsWorld) {
        atlas_error(
            "FixedJoint cannot have both parent and child as WorldBody");
        return;
    }

    if (!body1IsWorld && body2IsWorld) {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId1);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        auto *constraint = settings.Create(body, JPH::Body::sFixedToWorld);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else if (body1IsWorld && !body2IsWorld) {
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId2);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        auto *constraint = settings.Create(JPH::Body::sFixedToWorld, body);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        if (rb1 == nullptr || rb2 == nullptr)
            return;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);

        const JPH::BodyID bodies[2] = {bodyId1, bodyId2};
        JPH::BodyLockMultiWrite lock(
            world->physicsSystem.GetBodyLockInterface(), bodies, 2);

        JPH::Body *body1 = lock.GetBody(0);
        JPH::Body *body2 = lock.GetBody(1);
        if (body1 == nullptr || body2 == nullptr)
            return;

        auto *constraint = settings.Create(*body1, *body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    }
};

void bezel::HingeJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::HingeConstraintSettings settings;

    if (anchor == Position3d::invalid()) {
        atlas_error("HingeJoint requires an anchor point to be set.");
        return;
    }

    const JPH::RVec3 anchor_point(anchor.x, anchor.y, anchor.z);

    switch (space) {
    case Space::Global:
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        break;
    case Space::Local:
        settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
        break;
    }

    auto compute_normal_axis = [](JPH::Vec3 hinge_axis) {
        if (hinge_axis.LengthSq() < 1.0e-12f) {
            hinge_axis = JPH::Vec3::sAxisY();
        } else {
            hinge_axis = hinge_axis.Normalized();
        }

        JPH::Vec3 ref = JPH::Vec3::sAxisY();
        if (std::abs(hinge_axis.Dot(ref)) > 0.99f) {
            ref = JPH::Vec3::sAxisZ();
        }

        JPH::Vec3 normal = hinge_axis.Cross(ref);
        if (normal.LengthSq() < 1.0e-12f) {
            normal = hinge_axis.Cross(JPH::Vec3::sAxisX());
        }

        return normal.Normalized();
    };

    JPH::Vec3 hinge_axis_1(axis1.x, axis1.y, axis1.z);
    JPH::Vec3 hinge_axis_2(axis2.x, axis2.y, axis2.z);

    if (hinge_axis_1.LengthSq() < 1.0e-12f) {
        hinge_axis_1 = JPH::Vec3::sAxisY();
    } else {
        hinge_axis_1 = hinge_axis_1.Normalized();
    }

    if (hinge_axis_2.LengthSq() < 1.0e-12f) {
        hinge_axis_2 = hinge_axis_1;
    } else {
        hinge_axis_2 = hinge_axis_2.Normalized();
    }

    settings.mHingeAxis1 = hinge_axis_1;
    settings.mHingeAxis2 = hinge_axis_2;

    settings.mNormalAxis1 = compute_normal_axis(hinge_axis_1);
    settings.mNormalAxis2 = compute_normal_axis(hinge_axis_2);

    if (limits.enabled) {
        settings.mLimitsMax = limits.maxAngle;
        settings.mLimitsMin = limits.minAngle;
    }
    if (motor.enabled) {
        settings.mMotorSettings.SetForceLimit(motor.maxForce);
        settings.mMotorSettings.SetTorqueLimit(motor.maxTorque);
    }

    bool body1IsWorld = false;
    bool body2IsWorld = false;

    if (std::holds_alternative<WorldBody>(parent)) {
        body1IsWorld = true;
    }
    if (std::holds_alternative<WorldBody>(child)) {
        body2IsWorld = true;
    }

    if (body1IsWorld && body2IsWorld) {
        atlas_error(
            "HingeJoint cannot have both parent and child as WorldBody");
        return;
    }

    if (body1IsWorld || body2IsWorld) {
        // WorldBody doesn't have a meaningful local reference frame.
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
    }

    // In world space we need both anchor points to be set.
    if (settings.mSpace == JPH::EConstraintSpace::WorldSpace) {
        settings.mPoint1 = anchor_point;
        settings.mPoint2 = anchor_point;
    }

    if (!body1IsWorld && body2IsWorld) {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;

        if (rb1 == nullptr)
            return;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId1);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        if (settings.mSpace == JPH::EConstraintSpace::LocalToBodyCOM) {
            const JPH::RMat44 t1 = body.GetCenterOfMassTransform();
            settings.mPoint1 = t1.Inversed() * anchor_point;
            settings.mPoint2 = anchor_point;
        }
        auto *constraint = settings.Create(body, JPH::Body::sFixedToWorld);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else if (body1IsWorld && !body2IsWorld) {
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        if (rb2 == nullptr)
            return;

        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId2);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        if (settings.mSpace == JPH::EConstraintSpace::LocalToBodyCOM) {
            const JPH::RMat44 t2 = body.GetCenterOfMassTransform();
            settings.mPoint1 = anchor_point;
            settings.mPoint2 = t2.Inversed() * anchor_point;
        }
        auto *constraint = settings.Create(JPH::Body::sFixedToWorld, body);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        if (rb1 == nullptr || rb2 == nullptr)
            return;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);

        const JPH::BodyID bodies[2] = {bodyId1, bodyId2};
        JPH::BodyLockMultiWrite lock(
            world->physicsSystem.GetBodyLockInterface(), bodies, 2);

        JPH::Body *body1 = lock.GetBody(0);
        JPH::Body *body2 = lock.GetBody(1);
        if (body1 == nullptr || body2 == nullptr)
            return;

        if (settings.mSpace == JPH::EConstraintSpace::LocalToBodyCOM) {
            const JPH::RMat44 t1 = body1->GetCenterOfMassTransform();
            const JPH::RMat44 t2 = body2->GetCenterOfMassTransform();
            settings.mPoint1 = t1.Inversed() * anchor_point;
            settings.mPoint2 = t2.Inversed() * anchor_point;
        }

        auto *constraint = settings.Create(*body1, *body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    }
};

void bezel::SpringJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::DistanceConstraintSettings settings;

    if (anchor == Position3d::invalid()) {
        atlas_error("HingeJoint requires an anchor point to be set.");
        return;
    } else {
        settings.mPoint1 = JPH::Vec3(anchor.x, anchor.y, anchor.z);
    }
    if (anchorB == Position3d::invalid()) {
        atlas_error("SpringJoint requires anchorB point to be set.");
        return;
    } else {
        settings.mPoint2 = JPH::Vec3(anchorB.x, anchorB.y, anchorB.z);
    }

    switch (space) {
    case Space::Global:
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        break;
    case Space::Local:
        settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
        break;
    }

    if (useLimits) {
        settings.mMinDistance = minLength;
        settings.mMaxDistance = maxLength;
    } else {
        settings.mMinDistance = restLength;
        settings.mMaxDistance = restLength;
    }

    if (spring.enabled) {
        switch (spring.mode) {
        case bezel::SpringMode::FrequencyAndDamping: {
            settings.mLimitsSpringSettings.mMode =
                JPH::ESpringMode::FrequencyAndDamping;
        }
        case bezel::SpringMode::StiffnessAndDamping: {
            settings.mLimitsSpringSettings.mMode =
                JPH::ESpringMode::StiffnessAndDamping;
        }
        }

        if (spring.mode == bezel::SpringMode::FrequencyAndDamping) {
            settings.mLimitsSpringSettings.mFrequency = spring.frequencyHz;
            settings.mLimitsSpringSettings.mDamping = spring.dampingRatio;
        } else {
            settings.mLimitsSpringSettings.mStiffness = spring.stiffness;
            settings.mLimitsSpringSettings.mDamping = spring.damping;
        }
    }

    bool body1IsWorld = false;
    bool body2IsWorld = false;

    if (std::holds_alternative<WorldBody>(parent)) {
        body1IsWorld = true;
    }
    if (std::holds_alternative<WorldBody>(child)) {
        body2IsWorld = true;
    }

    if (body1IsWorld && body2IsWorld) {
        atlas_error(
            "FixedJoint cannot have both parent and child as WorldBody");
        return;
    }

    if (!body1IsWorld && body2IsWorld) {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId1);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        auto *constraint = settings.Create(body, JPH::Body::sFixedToWorld);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else if (body1IsWorld && !body2IsWorld) {
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                                bodyId2);

        if (!lock.Succeeded())
            return;

        JPH::Body &body = lock.GetBody();
        auto *constraint = settings.Create(JPH::Body::sFixedToWorld, body);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        if (rb1 == nullptr || rb2 == nullptr)
            return;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);

        const JPH::BodyID bodies[2] = {bodyId1, bodyId2};
        JPH::BodyLockMultiWrite lock(
            world->physicsSystem.GetBodyLockInterface(), bodies, 2);

        JPH::Body *body1 = lock.GetBody(0);
        JPH::Body *body2 = lock.GetBody(1);
        if (body1 == nullptr || body2 == nullptr)
            return;

        auto *constraint = settings.Create(*body1, *body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
            if (breakForce > 0.0f || breakTorque > 0.0f) {
                bezel::PhysicsWorld::BreakableConstraint entry;
                entry.constraint = joint;
                entry.breakForce = breakForce;
                entry.breakTorque = breakTorque;
                world->breakableConstraints.push_back(std::move(entry));
            }
        }
    }
};
#endif