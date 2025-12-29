//
// query.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Query-related functions for Jolt Implementation
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef BEZEL_JOLT_QUERY_H
#define BEZEL_JOLT_QUERY_H
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#ifndef BEZEL_NATIVE

#include <bezel/jolt/world.h>
#include <bezel/bezel.h>

struct PairKey {
    JPH::BodyID body1;
    JPH::BodyID body2;

    PairKey(JPH::BodyID b1, JPH::BodyID b2);

    bool operator==(const PairKey &other) const;
};

struct PairKeyHash {
    size_t operator()(const PairKey &key) const noexcept;
};

class GlobalContactListener final : public JPH::ContactListener {
  public:
    explicit GlobalContactListener(JPH::PhysicsSystem &physicsSystem)
        : physicsSystem(physicsSystem) {};

    JPH::ValidateResult
    OnContactValidate(const JPH::Body &, const JPH::Body &, JPH::RVec3Arg,
                      const JPH::CollideShapeResult &) override {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
                        const JPH::ContactManifold &inManifold,
                        JPH::ContactSettings &ioSettings) override;

    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2,
                            const JPH::ContactManifold &inManifold,
                            JPH::ContactSettings &ioSettings) override;

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;

    void dispatchEvents();

  private:
    void queueEnter(const JPH::BodyID &inBody1, const JPH::BodyID &inBody2);
    void queueExit(const JPH::BodyID &inBody1, const JPH::BodyID &inBody2);

    void fireOnCollisionEnter(const JPH::BodyID &inBody1,
                              const JPH::BodyID &inBody2);
    void fireOnCollisionExit(const JPH::BodyID &inBody1,
                             const JPH::BodyID &inBody2);

    JPH::PhysicsSystem &physicsSystem;

    std::unordered_set<PairKey, PairKeyHash> activePairs;

    std::vector<std::pair<JPH::BodyID, JPH::BodyID>> collisionEnterEvents;
    std::vector<std::pair<JPH::BodyID, JPH::BodyID>> collisionExitEvents;
};

class JoltCollisionDispatcher : public bezel::CollisionDispatcher {
  public:
    void setup(bezel::PhysicsWorld *world) override;
    void update(bezel::PhysicsWorld *world) override;

  private:
    std::shared_ptr<GlobalContactListener> contactListener;
};

#endif
#endif // BEZEL_JOLT_QUERY_H