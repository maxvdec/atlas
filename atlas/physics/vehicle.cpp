//
// vehicle.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Atlas Vehicle component (wraps Bezel Vehicle/Jolt
// VehicleConstraint)
//

#include "atlas/physics.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"

void Vehicle::atAttach() {
    if (!object) {
        return;
    }

    if (!object->rigidbody || !object->rigidbody->body) {
        atlas_warning("Vehicle attached to object without Rigidbody");
        return;
    }

    vehicle.chassis = object->rigidbody->body.get();
    vehicle.settings = settings;
}

void Vehicle::requestRecreate() { created = false; }

void Vehicle::beforePhysics() {
    if (!object || !Window::mainWindow || !Window::mainWindow->physicsWorld) {
        return;
    }

    if (!object->rigidbody || !object->rigidbody->body) {
        return;
    }

    vehicle.chassis = object->rigidbody->body.get();

    if (!created) {
        if (vehicle.isCreated()) {
            vehicle.destroy(Window::mainWindow->physicsWorld);
        }
        vehicle.settings = settings;

        vehicle.create(Window::mainWindow->physicsWorld);
        created = vehicle.isCreated();
    }

    if (vehicle.isCreated()) {
        vehicle.setDriverInput(forward, right, brake, handBrake);
    }
}
