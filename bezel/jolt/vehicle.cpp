//
// vehicle.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Vehicle (Jolt VehicleConstraint) wrapper for Bezel
//

#include <memory>
#include <algorithm>

#ifndef BEZEL_NATIVE
#include "bezel/bezel.h"
#include "bezel/jolt/world.h"
#include "atlas/tracer/log.h"

#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>

namespace {

static inline JPH::Vec3 toJoltVec3(const Position3d &v) {
    return JPH::Vec3(static_cast<float>(v.x), static_cast<float>(v.y),
                     static_cast<float>(v.z));
}

static inline float degToRad(float deg) { return deg * (JPH::JPH_PI / 180.0f); }

static inline JPH::ETransmissionMode toJolt(bezel::VehicleTransmissionMode m) {
    switch (m) {
    case bezel::VehicleTransmissionMode::Manual:
        return JPH::ETransmissionMode::Manual;
    case bezel::VehicleTransmissionMode::Auto:
    default:
        return JPH::ETransmissionMode::Auto;
    }
}

} // namespace

bezel::Vehicle::Vehicle(const Vehicle &other)
    : chassis(other.chassis), settings(other.settings) {}

bezel::Vehicle &bezel::Vehicle::operator=(const Vehicle &other) {
    if (this == &other) {
        return *this;
    }
    chassis = other.chassis;
    settings = other.settings;
    constraint = nullptr;
    controller = nullptr;
    collisionTester = nullptr;
    return *this;
}

bezel::Vehicle::Vehicle(Vehicle &&other) noexcept
    : chassis(other.chassis), settings(std::move(other.settings)),
      constraint(other.constraint), controller(other.controller),
      collisionTester(std::move(other.collisionTester)) {
    other.chassis = nullptr;
    other.constraint = nullptr;
    other.controller = nullptr;
    other.collisionTester = nullptr;
}

bezel::Vehicle &bezel::Vehicle::operator=(Vehicle &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    chassis = other.chassis;
    settings = std::move(other.settings);
    constraint = other.constraint;
    controller = other.controller;
    collisionTester = std::move(other.collisionTester);

    other.chassis = nullptr;
    other.constraint = nullptr;
    other.controller = nullptr;
    other.collisionTester = nullptr;
    return *this;
}

bool bezel::Vehicle::isCreated() const { return constraint != nullptr; }

void bezel::Vehicle::create(std::shared_ptr<PhysicsWorld> world) {
    if (!world) {
        atlas_error("Vehicle::create failed: world is null");
        return;
    }
    if (!chassis) {
        atlas_error("Vehicle::create failed: chassis is null");
        return;
    }
    if (chassis->id.joltId == bezel::INVALID_JOLT_ID) {
        atlas_error("Vehicle::create failed: chassis has invalid joltId (did "
                    "you call Rigidbody::create?)");
        return;
    }
    if (constraint != nullptr) {
        return;
    }
    if (settings.wheels.empty()) {
        atlas_error("Vehicle::create failed: no wheels configured");
        return;
    }

    const JPH::BodyID body_id(chassis->id.joltId);
    JPH::BodyLockWrite lock(world->physicsSystem.GetBodyLockInterface(),
                            body_id);
    if (!lock.Succeeded()) {
        atlas_error("Vehicle::create failed: could not lock chassis body");
        return;
    }

    JPH::Body &vehicle_body = lock.GetBody();

    JPH::VehicleConstraintSettings vc_settings;
    vc_settings.mUp = toJoltVec3(settings.up);
    vc_settings.mForward = toJoltVec3(settings.forward);

    float max_angle_rad = degToRad(settings.maxPitchRollAngleDeg);
    vc_settings.mMaxPitchRollAngle = max_angle_rad;

    JPH::Ref<JPH::WheeledVehicleControllerSettings> controller_settings =
        new JPH::WheeledVehicleControllerSettings();

    controller_settings->mEngine.mMaxTorque =
        settings.controller.engine.maxTorque;
    controller_settings->mEngine.mMinRPM = settings.controller.engine.minRPM;
    controller_settings->mEngine.mMaxRPM = settings.controller.engine.maxRPM;
    controller_settings->mEngine.mInertia = settings.controller.engine.inertia;
    controller_settings->mEngine.mAngularDamping =
        settings.controller.engine.angularDamping;

    controller_settings->mTransmission.mMode =
        toJolt(settings.controller.transmission.mode);

    controller_settings->mTransmission.mGearRatios.clear();
    controller_settings->mTransmission.mGearRatios.reserve(
        (JPH::uint)settings.controller.transmission.gearRatios.size());
    for (float ratio : settings.controller.transmission.gearRatios) {
        controller_settings->mTransmission.mGearRatios.push_back(ratio);
    }

    controller_settings->mTransmission.mReverseGearRatios.clear();
    controller_settings->mTransmission.mReverseGearRatios.reserve(
        (JPH::uint)settings.controller.transmission.reverseGearRatios.size());
    for (float ratio : settings.controller.transmission.reverseGearRatios) {
        controller_settings->mTransmission.mReverseGearRatios.push_back(ratio);
    }

    controller_settings->mTransmission.mSwitchTime =
        settings.controller.transmission.switchTime;
    controller_settings->mTransmission.mClutchReleaseTime =
        settings.controller.transmission.clutchReleaseTime;
    controller_settings->mTransmission.mSwitchLatency =
        settings.controller.transmission.switchLatency;
    controller_settings->mTransmission.mShiftUpRPM =
        settings.controller.transmission.shiftUpRPM;
    controller_settings->mTransmission.mShiftDownRPM =
        settings.controller.transmission.shiftDownRPM;
    controller_settings->mTransmission.mClutchStrength =
        settings.controller.transmission.clutchStrength;

    controller_settings->mDifferentials.clear();
    if (!settings.controller.differentials.empty()) {
        controller_settings->mDifferentials.reserve(
            (JPH::uint)settings.controller.differentials.size());
        for (const auto &diff : settings.controller.differentials) {
            JPH::VehicleDifferentialSettings d;
            d.mLeftWheel = diff.leftWheel;
            d.mRightWheel = diff.rightWheel;
            d.mDifferentialRatio = diff.differentialRatio;
            d.mLeftRightSplit = diff.leftRightSplit;
            d.mLimitedSlipRatio = diff.limitedSlipRatio;
            d.mEngineTorqueRatio = diff.engineTorqueRatio;
            controller_settings->mDifferentials.push_back(d);
        }
    } else {
        const int wheel_count = (int)settings.wheels.size();
        if (wheel_count >= 2) {
            int left = wheel_count >= 4 ? 2 : wheel_count - 2;
            int right = wheel_count >= 4 ? 3 : wheel_count - 1;
            JPH::VehicleDifferentialSettings d;
            d.mLeftWheel = left;
            d.mRightWheel = right;
            d.mEngineTorqueRatio = 1.0f;
            controller_settings->mDifferentials.push_back(d);
        }
    }
    controller_settings->mDifferentialLimitedSlipRatio =
        settings.controller.differentialLimitedSlipRatio;

    vc_settings.mWheels.reserve((JPH::uint)settings.wheels.size());
    for (const auto &w : settings.wheels) {
        JPH::Ref<JPH::WheelSettingsWV> ws = new JPH::WheelSettingsWV();
        ws->mPosition = toJoltVec3(w.position);
        ws->mSuspensionDirection = toJoltVec3(w.suspensionDirection);
        ws->mSteeringAxis = toJoltVec3(w.steeringAxis);
        ws->mWheelUp = toJoltVec3(w.wheelUp);
        ws->mWheelForward = toJoltVec3(w.wheelForward);

        ws->mSuspensionMinLength = w.suspensionMinLength;
        ws->mSuspensionMaxLength = w.suspensionMaxLength;
        ws->mSuspensionPreloadLength = w.suspensionPreloadLength;

        ws->mSuspensionSpring.mMode = JPH::ESpringMode::FrequencyAndDamping;
        ws->mSuspensionSpring.mFrequency = w.suspensionFrequencyHz;
        ws->mSuspensionSpring.mDamping = w.suspensionDampingRatio;

        ws->mRadius = w.radius;
        ws->mWidth = w.width;

        ws->mEnableSuspensionForcePoint = w.enableSuspensionForcePoint;
        ws->mSuspensionForcePoint = toJoltVec3(w.suspensionForcePoint);

        ws->mInertia = w.inertia;
        ws->mAngularDamping = w.angularDamping;
        ws->mMaxSteerAngle = degToRad(w.maxSteerAngleDeg);
        ws->mMaxBrakeTorque = w.maxBrakeTorque;
        ws->mMaxHandBrakeTorque = w.maxHandBrakeTorque;

        JPH::Ref<JPH::WheelSettings> base_ws = ws.GetPtr();
        vc_settings.mWheels.push_back(base_ws);
    }

    vc_settings.mController = controller_settings;

    auto *new_constraint =
        new JPH::VehicleConstraint(vehicle_body, vc_settings);

    const float max_slope_rad = degToRad(settings.maxSlopeAngleDeg);
    collisionTester = new JPH::VehicleCollisionTesterRay(
        bezel::jolt::layers::MOVING, JPH::Vec3::sAxisY(), max_slope_rad);
    new_constraint->SetVehicleCollisionTester(collisionTester);

    world->physicsSystem.AddConstraint(new_constraint);
    world->physicsSystem.AddStepListener(new_constraint);

    constraint = new_constraint;
    controller = dynamic_cast<JPH::WheeledVehicleController *>(
        new_constraint->GetController());

    if (controller == nullptr) {
        atlas_warning(
            "Vehicle created but controller is not WheeledVehicleController");
    }
}

void bezel::Vehicle::destroy(std::shared_ptr<PhysicsWorld> world) {
    if (!world) {
        constraint = nullptr;
        controller = nullptr;
        collisionTester = nullptr;
        return;
    }

    if (constraint == nullptr) {
        return;
    }

    world->physicsSystem.RemoveStepListener(constraint);
    world->physicsSystem.RemoveConstraint(constraint);
    delete constraint;

    constraint = nullptr;
    controller = nullptr;
    collisionTester = nullptr;
}

void bezel::Vehicle::setDriverInput(float forward, float right, float brake,
                                    float handBrake) {
    if (controller == nullptr) {
        return;
    }

    forward = std::max(-1.0f, std::min(1.0f, forward));
    right = std::max(-1.0f, std::min(1.0f, right));
    brake = std::max(0.0f, std::min(1.0f, brake));
    handBrake = std::max(0.0f, std::min(1.0f, handBrake));

    controller->SetDriverInput(forward, right, brake, handBrake);
}

#endif