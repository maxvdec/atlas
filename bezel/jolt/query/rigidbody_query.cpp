//
// rigidbody_query.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Queries to the Rigidbody for Jolt Implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/component.h"
#include "bezel/jolt/query.h"
#include "bezel/bezel.h"
#include "bezel/jolt/world.h"
#include <cstddef>
#include <atlas/tracer/log.h>
#include <iostream>

PairKey::PairKey(JPH::BodyID b1, JPH::BodyID b2) {
    if (b1.GetIndexAndSequenceNumber() < b2.GetIndexAndSequenceNumber()) {
        body1 = b1;
        body2 = b2;
    } else {
        body1 = b2;
        body2 = b1;
    }
}

bool PairKey::operator==(const PairKey &other) const {
    return body1 == other.body1 && body2 == other.body2;
}

size_t PairKeyHash::operator()(const PairKey &k) const noexcept {
    auto x = (JPH::uint64)k.body1.GetIndexAndSequenceNumber();
    auto y = (JPH::uint64)k.body2.GetIndexAndSequenceNumber();
    return (size_t)((x * 0x9E3779B185EBCA87ULL) ^
                    (y + 0x9E3779B185EBCA87ULL + (x << 6) + (x >> 2)));
}

void GlobalContactListener::OnContactAdded(
    const JPH::Body &inBody1, const JPH::Body &inBody2,
    const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) {
    PairKey key(inBody1.GetID(), inBody2.GetID());

    if (activePairs.insert(key).second) {
        queueEnter(inBody1.GetID(), inBody2.GetID());
    }
}

void GlobalContactListener::OnContactPersisted(
    const JPH::Body &inBody1, const JPH::Body &inBody2,
    const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) {
    // Currently not used
}

void GlobalContactListener::OnContactRemoved(
    const JPH::SubShapeIDPair &inSubShapePair) {
    PairKey key(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());

    auto it = activePairs.find(key);
    if (it != activePairs.end()) {
        activePairs.erase(it);
        queueExit(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());
    }
}

void GlobalContactListener::dispatchEvents() {
    for (auto &enterPair : collisionEnterEvents) {
        fireOnCollisionEnter(enterPair.first, enterPair.second);
    }
    for (auto &exitPair : collisionExitEvents) {
        fireOnCollisionExit(exitPair.first, exitPair.second);
    }
    collisionEnterEvents.clear();
    collisionExitEvents.clear();
}

void GlobalContactListener::queueEnter(const JPH::BodyID &inBody1,
                                       const JPH::BodyID &inBody2) {
    collisionEnterEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::queueExit(const JPH::BodyID &inBody1,
                                      const JPH::BodyID &inBody2) {
    collisionExitEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::fireOnCollisionEnter(const JPH::BodyID &inBody1,
                                                 const JPH::BodyID &inBody2) {
    auto it1 = bodyIdToRigidbodyMap.find(inBody1);
    auto it2 = bodyIdToRigidbodyMap.find(inBody2);

    if (it1 != bodyIdToRigidbodyMap.end() &&
        it2 != bodyIdToRigidbodyMap.end()) {
        bezel::Rigidbody *rigidbody1 = it1->second;
        bezel::Rigidbody *rigidbody2 = it2->second;

        auto objIt1 = atlas::gameObjects.find((int)rigidbody1->id.atlasId);
        auto objIt2 = atlas::gameObjects.find((int)rigidbody2->id.atlasId);

        GameObject *object1 =
            (objIt1 != atlas::gameObjects.end()) ? objIt1->second : nullptr;
        GameObject *object2 =
            (objIt2 != atlas::gameObjects.end()) ? objIt2->second : nullptr;

        if (object1 && object2) {
            object1->onCollisionEnter(object2);
            object2->onCollisionEnter(object1);
        } else {
            atlas_error(
                "One of the objects involved in collision enter is null.");
        }
    } else {
        atlas_error("One of the rigidbodies involved in collision enter is not "
                    "registered in the bodyIdToRigidbodyMap.");
    }
}

void GlobalContactListener::fireOnCollisionExit(const JPH::BodyID &inBody1,
                                                const JPH::BodyID &inBody2) {
    auto it1 = bodyIdToRigidbodyMap.find(inBody1);
    auto it2 = bodyIdToRigidbodyMap.find(inBody2);

    if (it1 != bodyIdToRigidbodyMap.end() &&
        it2 != bodyIdToRigidbodyMap.end()) {
        bezel::Rigidbody *rigidbody1 = it1->second;
        bezel::Rigidbody *rigidbody2 = it2->second;

        auto objIt1 = atlas::gameObjects.find((int)rigidbody1->id.atlasId);
        auto objIt2 = atlas::gameObjects.find((int)rigidbody2->id.atlasId);

        GameObject *object1 =
            (objIt1 != atlas::gameObjects.end()) ? objIt1->second : nullptr;
        GameObject *object2 =
            (objIt2 != atlas::gameObjects.end()) ? objIt2->second : nullptr;

        if (object1 && object2) {
            object1->onCollisionExit(object2);
            object2->onCollisionExit(object1);
        } else {
            atlas_error(
                "One of the objects involved in collision exit is null.");
        }
    } else {
        atlas_error("One of the rigidbodies involved in collision exit is not "
                    "registered in the bodyIdToRigidbodyMap.");
    }
}

void JoltCollisionDispatcher::setup(bezel::PhysicsWorld *world) {
    std::cout << "Setting up JoltCollisionDispatcher" << std::endl;
    this->contactListener =
        std::make_shared<GlobalContactListener>(world->physicsSystem);
    world->physicsSystem.SetContactListener(contactListener.get());
}

void JoltCollisionDispatcher::update(bezel::PhysicsWorld *world) {
    contactListener->dispatchEvents();
}

bezel::RaycastResult
bezel::Rigidbody::raycast(const Position3d &direction, float maxDistance,
                          std::shared_ptr<bezel::PhysicsWorld> world,
                          uint32_t ignoreBodyId) {
    Position3d origin = position;

    if (ignoreBodyId == bezel::INVALID_JOLT_ID) {
        ignoreBodyId = id.joltId;
    }

    return world->raycast(origin, direction, maxDistance, ignoreBodyId);
}

bezel::RaycastResult
bezel::Rigidbody::raycastAll(const Position3d &direction, float maxDistance,
                             std::shared_ptr<bezel::PhysicsWorld> world,
                             uint32_t ignoreBodyId) {
    Position3d origin = position;

    if (ignoreBodyId == bezel::INVALID_JOLT_ID) {
        ignoreBodyId = id.joltId;
    }

    return world->raycastAll(origin, direction, maxDistance, ignoreBodyId);
}