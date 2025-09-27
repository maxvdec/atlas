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
