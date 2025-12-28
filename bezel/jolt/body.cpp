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

void bezel::Rigidbody::create(std::shared_ptr<bezel::PhysicsWorld> world) {
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

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

void bezel::Rigidbody::refresh(std::shared_ptr<bezel::PhysicsWorld> world) {
    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyLockRead lock(world->physicsSystem.GetBodyLockInterface(),
                           joltBodyId);
    if (!lock.Succeeded())
        return;

    const JPH::Body &body = lock.GetBody();

    JPH::Vec3 position = body.GetPosition();
    JPH::Quat rotation = body.GetRotation();

    this->position =
        Position3d(position.GetX(), position.GetY(), position.GetZ());

    glm::quat glmRotation(rotation.GetW(), rotation.GetX(), rotation.GetY(),
                          rotation.GetZ());
    this->rotation = Rotation3d::fromGlmQuat(glmRotation);
}

void bezel::Rigidbody::setPosition(const Position3d &position,
                                   std::shared_ptr<PhysicsWorld> world) {
    if (id.joltId == 0) {
        this->position = position;
        return;
    }
    this->position = position;

    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    JPH::RVec3 joltPosition(position.x, position.y, position.z);
    bodyInterface.SetPosition(joltBodyId, joltPosition,
                              JPH::EActivation::Activate);
}

void bezel::Rigidbody::setRotation(const Rotation3d &rotation,
                                   std::shared_ptr<PhysicsWorld> world) {
    if (id.joltId == 0) {
        this->rotation = rotation;
        return;
    }
    this->rotation = rotation;

    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    glm::quat glmRotation = rotation.toGlmQuat();
    JPH::Quat joltRotation(glmRotation.w, glmRotation.x, glmRotation.y,
                           glmRotation.z);
    bodyInterface.SetRotation(joltBodyId, joltRotation,
                              JPH::EActivation::Activate);
}
