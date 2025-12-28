//
// rigidbody.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Rigidbody Atlas implementation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/physics.h"
#include "atlas/object.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <vector>

void Rigidbody::atAttach() {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }

    if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
        if (coreObject->rigidbody == nullptr) {
            coreObject->rigidbody = this;
        } else {
            atlas_warning(
                "CoreObject already has a Rigidbody component assigned.");
        }
    }

    body->id.atlasId = object->getId();
}

void Rigidbody::init() {
    if (body && Window::mainWindow && Window::mainWindow->physicsWorld) {
        body->position = object->getPosition();
        body->rotation = object->getRotation();
        body->create(Window::mainWindow->physicsWorld);
    } else {
        if (!body) {
            atlas_warning("Rigidbody initialization failed: missing body.");
        } else if (!Window::mainWindow) {
            atlas_warning(
                "Rigidbody initialization failed: missing main window.");
        } else if (!Window::mainWindow->physicsWorld) {
            atlas_warning(
                "Rigidbody initialization failed: missing physics world.");
        } else {
            atlas_warning("Rigidbody initialization failed: missing body or "
                          "physics world.");
        }
    }
}

void Rigidbody::addCapsuleCollider(float radius, float height) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }

    body->setCollider(std::make_shared<bezel::CapsuleCollider>(radius, height));
}

void Rigidbody::addBoxCollider(const Position3d &extents) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->setCollider(std::make_shared<bezel::BoxCollider>(extents / 2.0));
}

void Rigidbody::addSphereCollider(float radius) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->setCollider(std::make_shared<bezel::SphereCollider>(radius));
}

void Rigidbody::addMeshCollider() {
    if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
        std::vector<Position3d> vertices;
        vertices.reserve(coreObject->getVertices().size());
        for (const auto &vert : coreObject->getVertices()) {
            vertices.emplace_back(vert.position);
        }
        body->setCollider(std::make_shared<bezel::MeshCollider>(
            vertices, coreObject->indices));
        body->position = object->getPosition();
        body->rotation = object->getRotation();
        body->create(Window::mainWindow->physicsWorld);
    } else {
        atlas_warning(
            "MeshCollider can only be added to CoreObject instances.");
    }
}

void Rigidbody::beforePhysics() {
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    if (body->motionType == MotionType::Dynamic) {
        return;
    }

    body->setPosition(object->getPosition(), Window::mainWindow->physicsWorld);
    body->setRotation(object->getRotation(), Window::mainWindow->physicsWorld);

    body->applyProperties(Window::mainWindow->physicsWorld);
}

void Rigidbody::update(float dt) {
    (void)dt;
    if (!body || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    body->refresh(Window::mainWindow->physicsWorld);

    if (body->motionType == MotionType::Dynamic) {
        object->setPosition(body->position);
        if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
            coreObject->setRotationQuat(body->rotationQuat);
        } else {
            object->setRotation(body->rotation);
        }
    }
}

void Rigidbody::setFriction(float friction) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->friction = friction;
}

void Rigidbody::setMass(float mass) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->mass = mass;
}

void Rigidbody::setRestitution(float restitution) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->restitution = restitution;
}

void Rigidbody::setMotionType(MotionType motionType) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->motionType = motionType;
}

void Rigidbody::applyForce(const Position3d &force) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->force = force;
}

void Rigidbody::applyForceAtPoint(const Position3d &force,
                                  const Position3d &point) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->force = force;
    body->forcePoint = point;
}

void Rigidbody::applyImpulse(const Position3d &impulse) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->impulse = impulse;
}

void Rigidbody::setLinearVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->linearVelocity = velocity;
}

void Rigidbody::setAngularVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->angularVelocity = velocity;
}

void Rigidbody::addLinearVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->linearVelocity = velocity;
    body->addLinearVelocity = true;
}

void Rigidbody::addAngularVelocity(const Position3d &velocity) {
    if (!body) {
        body = std::make_shared<bezel::Rigidbody>();
    }
    body->angularVelocity = velocity;
    body->addAngularVelocity = true;
}