//
// body.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Body implementation for Jolt Physics
// Copyright (c) 2025 maxvdec
//

#include <bezel/bezel.h>
#include <bezel/jolt/world.h>

void bezel::Rigidbody::setCollider(std::shared_ptr<Collider> collider) {
    this->collider = collider;
}

void bezel::Rigidbody::create(bezel::PhysicsWorld &world) {
    JPH::BodyInterface &bodyInterface = world.physicsSystem.GetBodyInterface();

    auto shape = collider->getJoltShape();

    JPH::RVec3 joltPosition(position.x, position.y, position.z);

    glm::quat glmRotation = rotation.toGlmQuat();
    JPH::Quat joltRotation(glmRotation.w, glmRotation.x, glmRotation.y,
                           glmRotation.z);

    JPH::EMotionType joltMotionType;
    switch (motionType) {
    case MotionType::Static:
        joltMotionType = JPH::EMotionType::Static;
        break;
    case MotionType::Dynamic:
        joltMotionType = JPH::EMotionType::Dynamic;
        break;
    case MotionType::Kinematic:
        joltMotionType = JPH::EMotionType::Kinematic;
        break;
    default:
        joltMotionType = JPH::EMotionType::Dynamic;
        break;
    }

    JPH::BodyCreationSettings bodySettings(shape, joltPosition, joltRotation,
                                           joltMotionType,
                                           bezel::jolt::layers::MOVING);

    bodySettings.mFriction = friction;
    bodySettings.mRestitution = restitution;
    if (mass > 0.0f) {
        bodySettings.mOverrideMassProperties =
            JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = mass;
    }

    JPH::Body *joltBody = bodyInterface.CreateBody(bodySettings);
    JPH::BodyID joltBodyId = joltBody->GetID();
    this->id.joltId = joltBodyId.GetIndexAndSequenceNumber();
    bodyInterface.AddBody(joltBodyId, JPH::EActivation::Activate);
}