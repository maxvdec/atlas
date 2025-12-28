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
#include <iostream>

void Rigidbody::init() {
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

void Rigidbody::addCapsuleCollider(float radius, float height) {
    body->setCollider(std::make_shared<bezel::CapsuleCollider>(radius, height));
}

void Rigidbody::addBoxCollider(const Position3d &extents) {
    body->setCollider(std::make_shared<bezel::BoxCollider>(extents / 2.0));
}

void Rigidbody::addSphereCollider(float radius) {
    body->setCollider(std::make_shared<bezel::SphereCollider>(radius));
}

void Rigidbody::addMeshCollider() {
    if (auto *coreObject = dynamic_cast<CoreObject *>(object)) {
        body->setCollider(std::make_shared<bezel::MeshCollider>(
            coreObject->getVertices(), coreObject->indices));
    } else {
        atlas_warning(
            "MeshCollider can only be added to CoreObject instances.");
    }
}

void Rigidbody::beforePhysics() {}

void Rigidbody::update(float dt) {}