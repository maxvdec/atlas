//
// constraints.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Contraint implementations
// Copyright (c) 2025 Max Van den Eynde
//

#include "bezel/abstract.h"
#include <bezel/constraint.h>

bezel::vecN bezel::lcpGaussSeidel(const matN &A, const vecN &b) {
    const int N = b.number;
    vecN x(N);
    x.fill(0.0f);

    for (int iter = 0; iter < N; iter++) {
        for (int i = 0; i < N; i++) {
            float dx = (b[i] - dot(A.data[i], x)) / A.data[i][i];
            if (dx * 0.0f == dx * 0.0f) {
                x[i] = x[i] + dx;
            }
        }
    }
    return x;
}

bezel::matMN Constraint::getInverseMassMatrix() const {
    bezel::matMN invMassMatrix(12, 12);
    invMassMatrix.fill(0.0f);

    invMassMatrix.data[0][0] = bodyA->invMass;
    invMassMatrix.data[1][1] = bodyA->invMass;
    invMassMatrix.data[2][2] = bodyA->invMass;

    glm::mat3 inertiaTensorA =
        glm::inverse(bodyA->getInverseInertiaTensorWorldSpace());
    for (int i = 0; i < 3; i++) {
        invMassMatrix.data[i + 3][3 + 0] = inertiaTensorA[i][0];
        invMassMatrix.data[i + 3][3 + 1] = inertiaTensorA[i][1];
        invMassMatrix.data[i + 3][3 + 2] = inertiaTensorA[i][2];
    }

    invMassMatrix.data[6][6] = bodyB->invMass;
    invMassMatrix.data[7][7] = bodyB->invMass;
    invMassMatrix.data[8][8] = bodyB->invMass;

    glm::mat3 inertiaTensorB =
        glm::inverse(bodyB->getInverseInertiaTensorWorldSpace());
    for (int i = 0; i < 3; i++) {
        invMassMatrix.data[i + 9][9 + 0] = inertiaTensorB[i][0];
        invMassMatrix.data[i + 9][9 + 1] = inertiaTensorB[i][1];
        invMassMatrix.data[i + 9][9 + 2] = inertiaTensorB[i][2];
    }

    return invMassMatrix;
}

bezel::vecN Constraint::getVelocities() const {
    bezel::vecN velocities(12);
    velocities[0] = bodyA->linearVelocity.x;
    velocities[1] = bodyA->linearVelocity.y;
    velocities[2] = bodyA->linearVelocity.z;
    velocities[3] = bodyA->angularVelocity.x;
    velocities[4] = bodyA->angularVelocity.y;
    velocities[5] = bodyA->angularVelocity.z;

    velocities[6] = bodyB->linearVelocity.x;
    velocities[7] = bodyB->linearVelocity.y;
    velocities[8] = bodyB->linearVelocity.z;
    velocities[9] = bodyB->angularVelocity.x;
    velocities[10] = bodyB->angularVelocity.y;
    velocities[11] = bodyB->angularVelocity.z;

    return velocities;
}

void Constraint::applyImpulses(const bezel::vecN &impulses) {
    glm::vec3 forceInternalA(0.0f);
    glm::vec3 torqueInternalA(0.0f);
    glm::vec3 forceInternalB(0.0f);
    glm::vec3 torqueInternalB(0.0f);

    forceInternalA[0] = impulses[0];
    forceInternalA[1] = impulses[1];
    forceInternalA[2] = impulses[2];

    torqueInternalA[0] = impulses[3];
    torqueInternalA[1] = impulses[4];
    torqueInternalA[2] = impulses[5];

    forceInternalB[0] = impulses[6];
    forceInternalB[1] = impulses[7];
    forceInternalB[2] = impulses[8];

    torqueInternalB[0] = impulses[9];
    torqueInternalB[1] = impulses[10];
    torqueInternalB[2] = impulses[11];

    bodyA->applyLinearImpulse(forceInternalA);
    bodyA->applyAngularImpulse(torqueInternalA);

    bodyB->applyLinearImpulse(forceInternalB);
    bodyB->applyAngularImpulse(torqueInternalB);
}

void ConstraintDistance::preSolve(float dt) {
    const glm::vec3 worldAnchorA = bodyA->modelSpaceToWorldSpace(anchorA);
    const glm::vec3 worldAnchorB = bodyB->modelSpaceToWorldSpace(anchorB);

    const glm::vec3 r = worldAnchorB - worldAnchorA;
    const glm::vec3 ra = worldAnchorA - bodyA->getCenterOfMassWorldSpace();
    const glm::vec3 rb = worldAnchorB - bodyB->getCenterOfMassWorldSpace();
    const glm::vec3 a = worldAnchorA;
    const glm::vec3 b = worldAnchorB;

    jacobian.fill(0.0f);

    glm::vec3 J1 = (a - b) * 2.0f;
    jacobian.data[0][0] = J1.x;
    jacobian.data[0][1] = J1.y;
    jacobian.data[0][2] = J1.z;

    glm::vec3 J2 = glm::cross(ra, J1);
    jacobian.data[0][3] = J2.x;
    jacobian.data[0][4] = J2.y;
    jacobian.data[0][5] = J2.z;

    glm::vec3 J3 = (b - a) * 2.0f;
    jacobian.data[0][6] = J3.x;
    jacobian.data[0][7] = J3.y;
    jacobian.data[0][8] = J3.z;

    glm::vec3 J4 = glm::cross(rb, J3);
    jacobian.data[0][9] = J4.x;
    jacobian.data[0][10] = J4.y;
    jacobian.data[0][11] = J4.z;
}

void ConstraintDistance::solve() {
    bezel::matMN jacobianTranspose = jacobian.transpose();

    const bezel::vecN velocities = getVelocities();
    const bezel::matMN invMassMatrix = getInverseMassMatrix();
    const bezel::matMN J_W_Jt = jacobian * invMassMatrix * jacobianTranspose;
    bezel::vecN rhs = jacobian * velocities * -1.0f;

    const bezel::vecN lambda = bezel::lcpGaussSeidel(J_W_Jt, rhs);

    const bezel::vecN impulses = jacobianTranspose * lambda;
    applyImpulses(impulses);
}