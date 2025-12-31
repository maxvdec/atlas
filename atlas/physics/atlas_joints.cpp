//
// atlas_joints.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Joint implementation for Atlas
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "atlas/window.h"
#include "atlas/physics.h"
#include "bezel/bezel.h"
#include "atlas/tracer/log.h"
#include "opal/opal.h"
#include <memory>
#include <numbers>
#include <variant>

void FixedJoint::beforePhysics() {
    bool isFirstFrame = Window::mainWindow->firstFrame;
    if (isFirstFrame) {
        joint = std::make_shared<bezel::FixedJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error(
                    "FixedJoint parent GameObject has no Rigidbody component.");
                return;
            }
            joint->parent = parentObject->rigidbody->body.get();
        } else {
            joint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "FixedJoint child GameObject has no Rigidbody component.");
                return;
            }
            joint->child = childObject->rigidbody->body.get();
        } else {
            joint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "FixedJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            joint->space = bezel::Space::Global;
            break;
        case Space::Local:
            joint->space = bezel::Space::Local;
            break;
        }
        joint->anchor = anchor;
        joint->breakForce = breakForce;
        joint->breakTorque = breakTorque;
        joint->create(Window::mainWindow->physicsWorld);
    }
}

void FixedJoint::breakJoint() { joint->breakJoint(); }

void HingeJoint::beforePhysics() {
    bool isFirstFrame = Window::mainWindow->firstFrame;
    if (isFirstFrame) {
        joint = std::make_shared<bezel::HingeJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error(
                    "HingeJoint parent GameObject has no Rigidbody component.");
                return;
            }
            joint->parent = parentObject->rigidbody->body.get();
        } else {
            joint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "HingeJoint child GameObject has no Rigidbody component.");
                return;
            }
            joint->child = childObject->rigidbody->body.get();
        } else {
            joint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "HingeJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            joint->space = bezel::Space::Global;
            break;
        case Space::Local:
            joint->space = bezel::Space::Local;
            break;
        }
        joint->anchor = anchor;
        joint->breakForce = breakForce;
        joint->breakTorque = breakTorque;

        joint->axis1 = axis1;
        joint->axis2 = axis2;
        joint->limits.enabled = limits.enabled;
        joint->limits.minAngle =
            limits.minAngle * (std::numbers::pi_v<float> / 180.0f);
        joint->limits.maxAngle =
            limits.maxAngle * (std::numbers::pi_v<float> / 180.0f);
        joint->motor.enabled = motor.enabled;
        joint->motor.maxForce = motor.maxForce;
        joint->motor.maxTorque = motor.maxTorque;
        joint->create(Window::mainWindow->physicsWorld);
    }
}

void HingeJoint::breakJoint() { joint->breakJoint(); }

void SpringJoint::beforePhysics() {
    bool isFirstFrame = Window::mainWindow->firstFrame;
    if (isFirstFrame) {
        joint = std::make_shared<bezel::SpringJoint>();
        if (std::holds_alternative<GameObject *>(parent)) {
            GameObject *parentObject = *std::get_if<GameObject *>(&parent);
            if (!parentObject || !parentObject->rigidbody ||
                !parentObject->rigidbody->body) {
                atlas_error("SpringJoint parent GameObject has no Rigidbody "
                            "component.");
                return;
            }
            joint->parent = parentObject->rigidbody->body.get();
        } else {
            joint->parent = bezel::WorldBody{};
        }

        if (std::holds_alternative<GameObject *>(child)) {
            GameObject *childObject = *std::get_if<GameObject *>(&child);
            if (!childObject || !childObject->rigidbody ||
                !childObject->rigidbody->body) {
                atlas_error(
                    "SpringJoint child GameObject has no Rigidbody component.");
                return;
            }
            joint->child = childObject->rigidbody->body.get();
        } else {
            joint->child = bezel::WorldBody{};
        }

        if (std::holds_alternative<WorldBody>(parent) &&
            std::holds_alternative<WorldBody>(child)) {
            atlas_error(
                "SpringJoint cannot have both parent and child as WorldBody");
            return;
        }
        switch (space) {
        case Space::Global:
            joint->space = bezel::Space::Global;
            break;
        case Space::Local:
            joint->space = bezel::Space::Local;
            break;
        }
        joint->anchor = anchor;
        joint->breakForce = breakForce;
        joint->breakTorque = breakTorque;

        joint->restLength = restLength;
        joint->useLimits = useLimits;
        joint->minLength = minLength;
        joint->maxLength = maxLength;
        joint->spring.damping = spring.damping;
        joint->spring.enabled = spring.enabled;
        joint->spring.mode = static_cast<bezel::SpringMode>(spring.mode);
        joint->spring.frequencyHz = spring.frequencyHz;
        joint->spring.dampingRatio = spring.dampingRatio;
        joint->spring.stiffness = spring.stiffness;
        joint->spring.damping = spring.damping;
        joint->create(Window::mainWindow->physicsWorld);
    }
}

void SpringJoint::breakJoint() { joint->breakJoint(); }