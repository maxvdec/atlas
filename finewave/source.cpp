//
// source.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Source and emitter implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "finewave/audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>

const char *getALErrorStringSource(ALenum error) {
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
            std::cerr << "OpenAL error: " << getALErrorStringSource(err)       \
                      << " (" << err << ") at " << __FILE__ << ":" << __LINE__ \
                      << std::endl;                                            \
        }                                                                      \
    }

AudioSource::AudioSource() {
    ALCcontext *context = alcGetCurrentContext();
    if (!context) {
        std::cerr << "No OpenAL context is current when creating AudioSource"
                  << std::endl;
        throw std::runtime_error(
            "No OpenAL context is current when creating AudioSource");
    }

    alGenSources(1, &id);
    CHECK_AL_ERROR();

    if (!alIsSource(id)) {
        std::cerr << "Failed to generate OpenAL source (ID: " << id << ")"
                  << std::endl;
        throw std::runtime_error("Failed to generate OpenAL source");
    }
}

void AudioSource::setData(std::shared_ptr<AudioData> buffer) {
    if (!buffer) {
        std::cerr << "AudioData buffer is null" << std::endl;
        throw std::invalid_argument("AudioData buffer is null");
    }

    Id bufferId = buffer->getId();
    if (!alIsBuffer(bufferId)) {
        std::cerr << "Invalid OpenAL buffer ID: " << bufferId << std::endl;
        throw std::runtime_error("Invalid OpenAL buffer ID");
    }

    alSourcei(id, AL_BUFFER, bufferId);
    CHECK_AL_ERROR();
}

void AudioSource::fromFile(Resource resource) {
    try {
        auto audioData = AudioData::fromResource(resource);
        setData(audioData);
        CHECK_AL_ERROR();
    } catch (const std::exception &e) {
        std::cerr << "Failed to load audio from file: " << e.what()
                  << std::endl;
        throw;
    }
}

void AudioSource::play() {
    alSourcePlay(id);
    CHECK_AL_ERROR();
}
void AudioSource::pause() {
    alSourcePause(id);
    CHECK_AL_ERROR();
}
void AudioSource::stop() {
    alSourceStop(id);
    CHECK_AL_ERROR();
}

void AudioSource::setLooping(bool loop) {
    alSourcei(id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    CHECK_AL_ERROR();
}

void AudioSource::setVolume(float volume) {
    alSourcef(id, AL_GAIN, static_cast<ALfloat>(volume));
    CHECK_AL_ERROR();
}

void AudioSource::setPitch(float pitch) {
    alSourcef(id, AL_PITCH, static_cast<ALfloat>(pitch));
    CHECK_AL_ERROR();
}

void AudioSource::setPosition(Position3d position) {
    alSource3f(id, AL_POSITION, static_cast<ALfloat>(position.x),
               static_cast<ALfloat>(position.y),
               static_cast<ALfloat>(position.z));
}

void AudioSource::setVelocity(Magnitude3d velocity) {
    alSource3f(id, AL_VELOCITY, static_cast<ALfloat>(velocity.x),
               static_cast<ALfloat>(velocity.y),
               static_cast<ALfloat>(velocity.z));
}

bool AudioSource::isPlaying() const {
    ALint state;
    alGetSourcei(id, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void AudioSource::playFrom(float seconds) {
    alSourcef(id, AL_SEC_OFFSET, static_cast<ALfloat>(seconds));
    CHECK_AL_ERROR();
}

void AudioSource::disableSpatialization() {
    alSourcei(id, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(id, AL_POSITION, 0.0f, 0.0f, 0.0f);
}

AudioSource::~AudioSource() { alDeleteSources(1, &id); }