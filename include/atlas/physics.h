//
// physics.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: The physics components for Atlas
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_PHYSICS_H
#define ATLAS_PHYSICS_H

#include <bezel/bezel.h>
#include <atlas/component.h>
#include <memory>

class Rigidbody final : public Component {
  public:
    std::shared_ptr<bezel::Rigidbody> body;

    void init() override;
    void beforePhysics() override;
    void update(float dt) override;

    void addCapsuleCollider(float radius, float height);
    void addBoxCollider(const Position3d &extents);
    void addSphereCollider(float radius);
    void addMeshCollider();

    Rigidbody() = default;
};

#endif // ATLAS_PHYSICS_H