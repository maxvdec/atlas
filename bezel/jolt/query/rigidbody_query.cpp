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
#include <string>

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
    (void)inManifold;
    (void)ioSettings;
    if (inBody1.IsSensor() && inBody2.IsSensor()) {
        return;
    }
    if (inBody1.IsSensor() || inBody2.IsSensor()) {
        PairKey key(inBody1.GetID(), inBody2.GetID());
        if (activePairs.insert(key).second) {
            queueSignalEnter(inBody1.GetID(), inBody2.GetID());
        }
        return;
    }
    PairKey key(inBody1.GetID(), inBody2.GetID());

    if (activePairs.insert(key).second) {
        queueEnter(inBody1.GetID(), inBody2.GetID());
    }
}

void GlobalContactListener::OnContactPersisted(
    const JPH::Body &inBody1, const JPH::Body &inBody2,
    const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) {
    (void)inManifold;
    (void)ioSettings;
    PairKey key(inBody1.GetID(), inBody2.GetID());

    // Sensors can sometimes only show up as persisted contacts depending on
    // how the broadphase/narrowphase updates; handle them consistently.
    if (inBody1.IsSensor() && inBody2.IsSensor()) {
        return;
    }
    if (inBody1.IsSensor() || inBody2.IsSensor()) {
        if (activePairs.find(key) == activePairs.end()) {
            activePairs.insert(key);
            queueSignalEnter(inBody1.GetID(), inBody2.GetID());
        }
        return;
    }

    if (activePairs.find(key) == activePairs.end()) {
        activePairs.insert(key);
        queueEnter(inBody1.GetID(), inBody2.GetID());
    }
}

void GlobalContactListener::OnContactRemoved(
    const JPH::SubShapeIDPair &inSubShapePair) {

    PairKey key(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());

    // Don't lock bodies here: OnContactRemoved can be called from within the
    // physics update and taking locks can stall/hang.
    const JPH::BodyID body1 = inSubShapePair.GetBody1ID();
    const JPH::BodyID body2 = inSubShapePair.GetBody2ID();

    const auto itRb1 = bodyIdToRigidbodyMap.find(body1);
    const auto itRb2 = bodyIdToRigidbodyMap.find(body2);
    const bool body1IsSensor = (itRb1 != bodyIdToRigidbodyMap.end() &&
                                itRb1->second && itRb1->second->isSensor);
    const bool body2IsSensor = (itRb2 != bodyIdToRigidbodyMap.end() &&
                                itRb2->second && itRb2->second->isSensor);

    auto it = activePairs.find(key);
    if (it == activePairs.end()) {
        return;
    }

    activePairs.erase(it);

    if (body1IsSensor || body2IsSensor) {
        queueSignalExit(body1, body2);
    } else {
        queueExit(body1, body2);
    }
}

void GlobalContactListener::queueSignalEnter(const JPH::BodyID &inBody1,
                                             const JPH::BodyID &inBody2) {
    signalEnterEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::queueSignalExit(const JPH::BodyID &inBody1,
                                            const JPH::BodyID &inBody2) {
    signalExitEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::dispatchEvents() {
    for (auto &enterPair : collisionEnterEvents) {
        fireOnCollisionEnter(enterPair.first, enterPair.second);
    }
    for (auto &exitPair : collisionExitEvents) {
        fireOnCollisionExit(exitPair.first, exitPair.second);
    }
    for (auto &persistPair : collisionPersistEvents) {
        fireOnCollisionPersist(persistPair.first, persistPair.second);
    }
    for (auto &signalEnterPair : signalEnterEvents) {
        fireOnSignalEnter(signalEnterPair.first, signalEnterPair.second);
    }
    for (auto &signalExitPair : signalExitEvents) {
        fireOnSignalExit(signalExitPair.first, signalExitPair.second);
    }
    collisionEnterEvents.clear();
    collisionExitEvents.clear();
    collisionPersistEvents.clear();
    signalEnterEvents.clear();
    signalExitEvents.clear();
}

void GlobalContactListener::queueEnter(const JPH::BodyID &inBody1,
                                       const JPH::BodyID &inBody2) {
    collisionEnterEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::queueExit(const JPH::BodyID &inBody1,
                                      const JPH::BodyID &inBody2) {
    collisionExitEvents.emplace_back(inBody1, inBody2);
}

void GlobalContactListener::queuePersist(const JPH::BodyID &inBody1,
                                         const JPH::BodyID &inBody2) {
    collisionPersistEvents.emplace_back(inBody1, inBody2);
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

void GlobalContactListener::fireOnSignalEnter(const JPH::BodyID &inBody1,
                                              const JPH::BodyID &inBody2) {
    auto it1 = bodyIdToRigidbodyMap.find(inBody1);
    auto it2 = bodyIdToRigidbodyMap.find(inBody2);

    if (it1 != bodyIdToRigidbodyMap.end() &&
        it2 != bodyIdToRigidbodyMap.end()) {
        bezel::Rigidbody *rigidbody1 = it1->second;
        bezel::Rigidbody *rigidbody2 = it2->second;

        std::string event;
        if (rigidbody1->isSensor) {
            event = rigidbody1->sensorSignal;
        } else if (rigidbody2->isSensor) {
            event = rigidbody2->sensorSignal;
        } else {
            return;
        }

        auto objIt1 = atlas::gameObjects.find((int)rigidbody1->id.atlasId);
        auto objIt2 = atlas::gameObjects.find((int)rigidbody2->id.atlasId);

        GameObject *object1 =
            (objIt1 != atlas::gameObjects.end()) ? objIt1->second : nullptr;
        GameObject *object2 =
            (objIt2 != atlas::gameObjects.end()) ? objIt2->second : nullptr;

        if (object1 && object2) {
            if (rigidbody1->isSensor) {
                object2->onSignalRecieve(event, object1);
            } else if (rigidbody2->isSensor) {
                object1->onSignalRecieve(event, object2);
            }
        } else {
            atlas_error(
                "One of the objects involved in collision enter is null.");
        }
    } else {
        atlas_error("One of the rigidbodies involved in collision enter is not "
                    "registered in the bodyIdToRigidbodyMap.");
    }
}

void GlobalContactListener::fireOnSignalExit(const JPH::BodyID &inBody1,
                                             const JPH::BodyID &inBody2) {
    auto it1 = bodyIdToRigidbodyMap.find(inBody1);
    auto it2 = bodyIdToRigidbodyMap.find(inBody2);

    if (it1 != bodyIdToRigidbodyMap.end() &&
        it2 != bodyIdToRigidbodyMap.end()) {
        bezel::Rigidbody *rigidbody1 = it1->second;
        bezel::Rigidbody *rigidbody2 = it2->second;

        std::string event;
        if (rigidbody1->isSensor) {
            event = rigidbody1->sensorSignal;
        } else if (rigidbody2->isSensor) {
            event = rigidbody2->sensorSignal;
        } else {
            return;
        }

        auto objIt1 = atlas::gameObjects.find((int)rigidbody1->id.atlasId);
        auto objIt2 = atlas::gameObjects.find((int)rigidbody2->id.atlasId);

        GameObject *object1 =
            (objIt1 != atlas::gameObjects.end()) ? objIt1->second : nullptr;
        GameObject *object2 =
            (objIt2 != atlas::gameObjects.end()) ? objIt2->second : nullptr;

        if (object1 && object2) {
            if (rigidbody1->isSensor) {
                object2->onSignalEnd(event, object1);
            } else if (rigidbody2->isSensor) {
                object1->onSignalEnd(event, object2);
            }
        } else {
            atlas_error(
                "One of the objects involved in collision enter is null.");
        }
    } else {
        atlas_error("One of the rigidbodies involved in collision enter is not "
                    "registered in the bodyIdToRigidbodyMap.");
    }
}

void GlobalContactListener::fireOnCollisionPersist(const JPH::BodyID &inBody1,
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
            object1->onCollisionStay(object2);
            object2->onCollisionStay(object1);
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
    (void)world;
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

bezel::OverlapResult
bezel::Rigidbody::overlap(std::shared_ptr<bezel::PhysicsWorld> world,
                          std::shared_ptr<bezel::Collider> collider,
                          const Position3d &position,
                          const Rotation3d &rotation, uint32_t ignoreBodyId) {
    if (ignoreBodyId == bezel::INVALID_JOLT_ID) {
        ignoreBodyId = id.joltId;
    }

    return world->overlap(world, collider, position, rotation, ignoreBodyId);
}

bezel::SweepResult
bezel::Rigidbody::sweep(std::shared_ptr<bezel::PhysicsWorld> world,
                        std::shared_ptr<bezel::Collider> collider,
                        const Position3d &direction, Position3d &endPosition,
                        uint32_t ignoreBodyId) {
    if (ignoreBodyId == bezel::INVALID_JOLT_ID) {
        ignoreBodyId = id.joltId;
    }

    return world->sweep(world, collider, position, rotation, direction,
                        endPosition, ignoreBodyId);
}

bezel::SweepResult
bezel::Rigidbody::sweepAll(std::shared_ptr<bezel::PhysicsWorld> world,
                           std::shared_ptr<bezel::Collider> collider,
                           const Position3d &direction, Position3d &endPosition,
                           uint32_t ignoreBodyId) {
    if (ignoreBodyId == bezel::INVALID_JOLT_ID) {
        ignoreBodyId = id.joltId;
    }

    return world->sweepAll(world, collider, position, rotation, direction,
                           endPosition, ignoreBodyId);
}