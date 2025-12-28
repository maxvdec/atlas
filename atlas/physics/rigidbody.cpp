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
#include <iostream>
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
        body->create(Window::mainWindow->physicsWorld);
    } else {
        atlas_warning(
            "MeshCollider can only be added to CoreObject instances.");
    }
}

void Rigidbody::beforePhysics() {
    body->setPosition(object->getPosition(), Window::mainWindow->physicsWorld);
    body->setRotation(object->getRotation(), Window::mainWindow->physicsWorld);
}

void Rigidbody::update(float dt) {
    body->refresh(Window::mainWindow->physicsWorld);
    object->setPosition(body->position);
    object->setRotation(body->rotation);
}