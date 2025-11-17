//
// engine.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Engine implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "finewave/audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include "atlas/units.h"

const char *getALErrorStringSourceEngine(ALenum error) {
    switch (error) {
    case AL_NO_ERROR:
        return "AL_NO_ERROR";
    case AL_INVALID_NAME:
        return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
        return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
        return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
        return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
        return "AL_OUT_OF_MEMORY";
    default:
        return "UNKNOWN_AL_ERROR";
    }
}

#define CHECK_AL_ERROR()                                                       \
    {                                                                          \
        ALenum err = alGetError();                                             \
        if (err != AL_NO_ERROR) {                                              \
            std::cerr << "OpenAL error: " << getALErrorStringSourceEngine(err) \
                      << " (" << err << ") at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                                            \
        }                                                                      \
    }

bool AudioEngine::initialize() {
    ALCdevice *device = alcOpenDevice(nullptr);
    if (device == nullptr) {
        return false;
    }

    ALCenum alcError = alcGetError(device);
    if (alcError != ALC_NO_ERROR) {
        std::cout << "OpenAL error: " << alcError << std::endl;
        alcCloseDevice(device);
        return false;
    }

    ALCcontext *context = alcCreateContext(device, nullptr);
    if (context == nullptr) {
        alcCloseDevice(device);
        return false;
    }
    alcError = alcGetError(device);
    if (alcError != ALC_NO_ERROR) {
        std::cerr << "ALC error after creating context: " << alcError
                  << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        return false;
    }

    if (!alcMakeContextCurrent(context)) {
        alcDestroyContext(context);
        alcCloseDevice(device);
        return false;
    }

    alcError = alcGetError(device);
    if (alcError != ALC_NO_ERROR) {
        std::cerr << "ALC error after making context current: " << alcError
                  << std::endl;
        alcDestroyContext(context);
        alcCloseDevice(device);
        return false;
    }

    const ALCchar *deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
    if (deviceName) {
        this->deviceName = std::string(deviceName);
    } else {
        this->deviceName = "Unknown Device";
    }

    CHECK_AL_ERROR();

    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    ALfloat orientation[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
    alListenerfv(AL_ORIENTATION, orientation);

    CHECK_AL_ERROR();

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    return true;
}

void AudioEngine::shutdown() {
    ALCcontext *context = alcGetCurrentContext();
    ALCdevice *device = alcGetContextsDevice(context);

    if (context != nullptr) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
    }

    if (device != nullptr) {
        alcCloseDevice(device);
    }
}

void AudioEngine::setListenerPosition(Position3d position) {
    alListener3f(AL_POSITION, static_cast<ALfloat>(position.x),
                 static_cast<ALfloat>(position.y),
                 static_cast<ALfloat>(position.z));
}

void AudioEngine::setListenerOrientation(Magnitude3d forward, Normal3d up) {
    ALfloat orientation[6] = {
        static_cast<ALfloat>(forward.x), static_cast<ALfloat>(forward.y),
        static_cast<ALfloat>(forward.z), static_cast<ALfloat>(up.x),
        static_cast<ALfloat>(up.y),      static_cast<ALfloat>(up.z)};
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioEngine::setMasterVolume(float volume) {
    alListenerf(AL_GAIN, volume);
}

void AudioEngine::setListenerVelocity(Magnitude3d velocity) {
    alListener3f(AL_VELOCITY, static_cast<ALfloat>(velocity.x),
                 static_cast<ALfloat>(velocity.y),
                 static_cast<ALfloat>(velocity.z));
}