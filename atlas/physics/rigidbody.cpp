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

void Rigidbody::beforePhysics() {}

void Rigidbody::update(float dt) {}