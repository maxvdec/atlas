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
#include "atlas/tracer/log.h"

void bezel::FixedJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::FixedConstraintSettings settings;

    world->joints.push_back(this);

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
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock1(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId1);
        JPH::BodyLockWrite lock2(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId2);

        if (!lock1.Succeeded() || !lock2.Succeeded())
            return;

        JPH::Body &body1 = lock1.GetBody();
        JPH::Body &body2 = lock2.GetBody();
        auto *constraint = settings.Create(body1, body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
        }
    }
};

void bezel::HingeJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::HingeConstraintSettings settings;

    world->joints.push_back(this);

    if (anchor == Position3d::invalid()) {
        atlas_error("HingeJoint requires an anchor point to be set.");
        return;
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

    settings.mHingeAxis1 = JPH::Vec3(axis1.x, axis1.y, axis1.z);
    settings.mHingeAxis2 = JPH::Vec3(axis2.x, axis2.y, axis2.z);

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
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock1(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId1);
        JPH::BodyLockWrite lock2(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId2);

        if (!lock1.Succeeded() || !lock2.Succeeded())
            return;

        JPH::Body &body1 = lock1.GetBody();
        JPH::Body &body2 = lock2.GetBody();
        auto *constraint = settings.Create(body1, body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
        }
    }
};

void bezel::SpringJoint::create(std::shared_ptr<PhysicsWorld> world) {
    JPH::DistanceConstraintSettings settings;

    world->joints.push_back(this);

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
        }
    } else {
        Rigidbody *rb1 = std::get_if<Rigidbody *>(&parent)
                             ? *std::get_if<Rigidbody *>(&parent)
                             : nullptr;
        Rigidbody *rb2 = std::get_if<Rigidbody *>(&child)
                             ? *std::get_if<Rigidbody *>(&child)
                             : nullptr;

        auto bodyId1 = JPH::BodyID(rb1->id.joltId);
        auto bodyId2 = JPH::BodyID(rb2->id.joltId);
        JPH::BodyLockWrite lock1(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId1);
        JPH::BodyLockWrite lock2(world->physicsSystem.GetBodyLockInterface(),
                                 bodyId2);

        if (!lock1.Succeeded() || !lock2.Succeeded())
            return;

        JPH::Body &body1 = lock1.GetBody();
        JPH::Body &body2 = lock2.GetBody();
        auto *constraint = settings.Create(body1, body2);
        joint = constraint;
        if (joint) {
            world->physicsSystem.AddConstraint(joint);
        }
    }
};
#endif