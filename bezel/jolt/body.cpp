//
// body.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Body implementation for Jolt Physics
// Copyright (c) 2025 maxvdec
//

#include "atlas/window.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include <bezel/bezel.h>
#include <bezel/jolt/world.h>
#include <cmath>
#include <memory>

void bezel::Rigidbody::setCollider(std::shared_ptr<Collider> collider) {
    this->collider = collider;
}

void bezel::Rigidbody::create(std::shared_ptr<bezel::PhysicsWorld> world) {
    if (this->id.joltId != bezel::INVALID_JOLT_ID) {
        destroy(world);
    }
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    auto shape = collider->getJoltShape();

    JPH::RVec3 joltPosition(position.x, position.y, position.z);

    glm::quat glmRotation = glm::normalize(rotation.toGlmQuat());
    rotationQuat = glmRotation;
    JPH::Quat joltRotation(glmRotation.x, glmRotation.y, glmRotation.z,
                           glmRotation.w);

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

    // Choose the object layer based on motion type + sensor status.
    // This must match the filters in `bezel/jolt/world.cpp`.
    JPH::ObjectLayer objectLayer = bezel::jolt::layers::MOVING;
    if (isSensor) {
        objectLayer = bezel::jolt::layers::SENSOR;
    } else if (joltMotionType == JPH::EMotionType::Static) {
        objectLayer = bezel::jolt::layers::NON_MOVING;
    } else {
        objectLayer = bezel::jolt::layers::MOVING;
    }

    JPH::BodyCreationSettings bodySettings(shape, joltPosition, joltRotation,
                                           joltMotionType, objectLayer);

    bodySettings.mFriction = friction;
    bodySettings.mRestitution = restitution;
    bodySettings.mLinearDamping = linearDamping;
    bodySettings.mAngularDamping = angularDamping;
    bodySettings.mIsSensor = isSensor;
    if (mass > 0.0f) {
        bodySettings.mOverrideMassProperties =
            JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = mass;
    }

    JPH::Body *joltBody = bodyInterface.CreateBody(bodySettings);
    JPH::BodyID joltBodyId = joltBody->GetID();
    this->id.joltId = joltBodyId.GetIndexAndSequenceNumber();
    bodyInterface.AddBody(joltBodyId, JPH::EActivation::Activate);

    applyProperties(world);
    bodyIdToRigidbodyMap[joltBodyId] = this;
}

void bezel::Rigidbody::refresh(std::shared_ptr<bezel::PhysicsWorld> world) {
    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyLockRead lock(world->physicsSystem.GetBodyLockInterface(),
                           joltBodyId);
    if (!lock.Succeeded())
        return;

    const JPH::Body &body = lock.GetBody();

    JPH::Vec3 linearVelocity = body.GetLinearVelocity();
    JPH::Vec3 position = body.GetPosition();
    JPH::Quat rotation = body.GetRotation();

    bool needsCCD =
        Window::mainWindow->getDeltaTime() * linearVelocity.Length() >
        this->collider->getMinExtent() / 2.0f;

    lock.ReleaseLock();

    if (needsCCD) {
        atlas_log(
            "[JOLT] Enabling linear cast for fast moving body with Object ID " +
            std::to_string(this->id.atlasId));
        world->physicsSystem.GetBodyInterface().SetMotionQuality(
            joltBodyId, JPH::EMotionQuality::LinearCast);
    }

    this->position =
        Position3d(position.GetX(), position.GetY(), position.GetZ());

    glm::quat glmRotation(rotation.GetW(), rotation.GetX(), rotation.GetY(),
                          rotation.GetZ());

    if (glm::dot(rotationQuat, glmRotation) < 0.0f) {
        glmRotation = -glmRotation;
    }

    rotationQuat = glm::normalize(glmRotation);

    Rotation3d next = Rotation3d::fromGlmQuat(rotationQuat);

    auto unwrapDegrees = [](float prev, float current) {
        float delta = std::remainder(current - prev, 360.0f);
        return prev + delta;
    };

    next.pitch = unwrapDegrees(this->rotation.pitch, next.pitch);
    next.yaw = unwrapDegrees(this->rotation.yaw, next.yaw);
    next.roll = unwrapDegrees(this->rotation.roll, next.roll);

    this->rotation = next;
}

void bezel::Rigidbody::setPosition(const Position3d &position,
                                   std::shared_ptr<PhysicsWorld> world) {
    if (id.joltId == INVALID_JOLT_ID) {
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
    if (id.joltId == INVALID_JOLT_ID) {
        this->rotation = rotation;
        return;
    }
    this->rotation = rotation;

    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();

    glm::quat glmRotation = glm::normalize(rotation.toGlmQuat());
    rotationQuat = glmRotation;
    JPH::Quat joltRotation(glmRotation.x, glmRotation.y, glmRotation.z,
                           glmRotation.w);
    bodyInterface.SetRotation(joltBodyId, joltRotation,
                              JPH::EActivation::Activate);
}

void bezel::Rigidbody::destroy(std::shared_ptr<bezel::PhysicsWorld> world) {
    if (id.joltId == INVALID_JOLT_ID) {
        return;
    }

    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();
    auto joltBodyId = JPH::BodyID(id.joltId);

    bodyIdToRigidbodyMap.erase(joltBodyId);

    bodyInterface.RemoveBody(joltBodyId);
    bodyInterface.DestroyBody(joltBodyId);
    id.joltId = INVALID_JOLT_ID;
}

void bezel::Rigidbody::applyProperties(
    std::shared_ptr<bezel::PhysicsWorld> world) {
    if (id.joltId == INVALID_JOLT_ID) {
        return;
    }

    auto joltBodyId = JPH::BodyID(id.joltId);
    JPH::BodyInterface &bodyInterface = world->physicsSystem.GetBodyInterface();
    if (linearVelocity != Position3d{-1.0f, -1.0f, -1.0f}) {
        if (addLinearVelocity) {
            JPH::Vec3 linearVelocityVec =
                bodyInterface.GetLinearVelocity(joltBodyId);
            bodyInterface.SetLinearVelocity(
                joltBodyId, JPH::Vec3(linearVelocity.x, linearVelocity.y,
                                      linearVelocity.z) +
                                linearVelocityVec);
            addLinearVelocity = false;
            linearVelocity = Position3d{-1.0f, -1.0f, -1.0f};
        } else {
            bodyInterface.SetLinearVelocity(
                joltBodyId, JPH::Vec3(linearVelocity.x, linearVelocity.y,
                                      linearVelocity.z));
            linearVelocity = Position3d{-1.0f, -1.0f, -1.0f};
        }
    }

    if (angularVelocity != Position3d{-1.0f, -1.0f, -1.0f}) {
        if (addAngularVelocity) {
            JPH::Vec3 angularVelocityVec =
                bodyInterface.GetAngularVelocity(joltBodyId);
            bodyInterface.SetAngularVelocity(
                joltBodyId, JPH::Vec3(angularVelocity.x, angularVelocity.y,
                                      angularVelocity.z) +
                                angularVelocityVec);
            addAngularVelocity = false;
            angularVelocity = Position3d{-1.0f, -1.0f, -1.0f};
        } else {
            bodyInterface.SetAngularVelocity(
                joltBodyId, JPH::Vec3(angularVelocity.x, angularVelocity.y,
                                      angularVelocity.z));
            angularVelocity = Position3d{-1.0f, -1.0f, -1.0f};
        }
    }

    if (impulse != Position3d{0.0f, 0.0f, 0.0f}) {
        bodyInterface.AddImpulse(
            joltBodyId, JPH::Vec3(impulse.x, impulse.y, impulse.z),
            JPH::Vec3(forcePoint.x, forcePoint.y, forcePoint.z));
        impulse = Position3d{0.0f, 0.0f, 0.0f};
        forcePoint = Position3d{0.0f, 0.0f, 0.0f};
    }

    if (force != Position3d{0.0f, 0.0f, 0.0f}) {
        if (forcePoint == Position3d{0.0f, 0.0f, 0.0f}) {
            bodyInterface.AddForce(joltBodyId,
                                   JPH::Vec3(force.x, force.y, force.z));
            force = Position3d{0.0f, 0.0f, 0.0f};
        } else {
            bodyInterface.AddForce(
                joltBodyId, JPH::Vec3(force.x, force.y, force.z),
                JPH::Vec3(forcePoint.x, forcePoint.y, forcePoint.z));
            force = Position3d{0.0f, 0.0f, 0.0f};
            forcePoint = Position3d{0.0f, 0.0f, 0.0f};
        }
    }
}