//
// source.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Source and emitter implementation
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/units.h"
#include "finewave/audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

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

    // Generate BOTH sources
    alGenSources(1, &id);
    CHECK_AL_ERROR();

    if (!alIsSource(id)) {
        std::cerr << "Failed to generate OpenAL source (ID: " << id << ")"
                  << std::endl;
        throw std::runtime_error("Failed to generate OpenAL source");
    }

    // Generate the mono source for spatialization
    alGenSources(1, &monoId);
    CHECK_AL_ERROR();

    if (!alIsSource(monoId)) {
        std::cerr << "Failed to generate OpenAL mono source (ID: " << monoId
                  << ")" << std::endl;
        alDeleteSources(1, &id); // Clean up the first source
        throw std::runtime_error("Failed to generate OpenAL mono source");
    }
}

void AudioSource::setData(const std::shared_ptr<AudioData> &buffer) {
    if (!buffer) {
        std::cerr << "AudioData buffer is null" << std::endl;
        throw std::invalid_argument("AudioData buffer is null");
    }

    this->data = buffer;

    Id bufferId = buffer->getId();
    if (!alIsBuffer(bufferId)) {
        std::cerr << "Invalid OpenAL buffer ID: " << bufferId << std::endl;
        throw std::runtime_error("Invalid OpenAL buffer ID");
    }

    // Bind the original buffer to the main source
    alSourcei(id, AL_BUFFER, bufferId);
    CHECK_AL_ERROR();

    if (!buffer->isMono) {
        const auto data = buffer->data;
        std::vector<char> monoData;
        monoData.resize(data.size() / 2);
        for (size_t i = 0; i < monoData.size() / sizeof(int16_t); ++i) {
            int16_t leftSample =
                reinterpret_cast<const int16_t *>(data.data())[i * 2];
            int16_t rightSample =
                reinterpret_cast<const int16_t *>(data.data())[(i * 2) + 1];
            int16_t monoSample =
                static_cast<int16_t>((leftSample + rightSample) / 2);
            reinterpret_cast<int16_t *>(monoData.data())[i] = monoSample;
        }
        ALuint monoBufferId;
        alGenBuffers(1, &monoBufferId);
        CHECK_AL_ERROR();
        if (!alIsBuffer(monoBufferId)) {
            std::cerr << "Failed to generate OpenAL mono buffer" << std::endl;
            throw std::runtime_error("Failed to generate OpenAL mono buffer");
        }
        alBufferData(monoBufferId, AL_FORMAT_MONO16, monoData.data(),
                     static_cast<ALsizei>(monoData.size()), buffer->sampleRate);
        CHECK_AL_ERROR();
        this->monoData = std::make_shared<AudioData>();
        this->monoData->id = monoBufferId;
        this->monoData->isMono = true;
        this->monoData->data = std::move(monoData);
        this->monoData->sampleRate = buffer->sampleRate;
        this->monoData->resource = buffer->resource;

        // Bind the mono buffer to the mono source
        alSourcei(monoId, AL_BUFFER, monoBufferId);
        CHECK_AL_ERROR();
    } else {
        this->monoData = buffer;
        // Bind the mono buffer to the mono source
        alSourcei(monoId, AL_BUFFER, bufferId);
        CHECK_AL_ERROR();
    }
}

void AudioSource::fromFile(Resource resource) {
    try {
        auto audioData = AudioData::fromResource(std::move(resource));
        setData(audioData);
        CHECK_AL_ERROR();
    } catch (const std::exception &e) {
        std::cerr << "Failed to load audio from file: " << e.what()
                  << std::endl;
        throw;
    }
}

void AudioSource::play() {
    if (isSpatialized && monoData) {
        alSourceStop(id);
        alSourcePlay(monoId);
        CHECK_AL_ERROR();
        return;
    }
    alSourceStop(monoId);
    alSourcePlay(id);
    CHECK_AL_ERROR();
}

void AudioSource::pause() {
    if (this->isSpatialized && monoData) {
        alSourcePause(monoId);
        CHECK_AL_ERROR();
        return;
    }
    alSourcePause(id);
    CHECK_AL_ERROR();
}

void AudioSource::stop() {
    if (this->isSpatialized && monoData) {
        alSourceStop(monoId);
        CHECK_AL_ERROR();
        return;
    }
    alSourceStop(id);
    CHECK_AL_ERROR();
}

void AudioSource::setLooping(bool loop) {
    alSourcei(id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    if (monoData) {
        alSourcei(monoId, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }
    CHECK_AL_ERROR();
}

void AudioSource::setVolume(float volume) {
    alSourcef(id, AL_GAIN, static_cast<ALfloat>(volume));
    if (monoData) {
        alSourcef(monoId, AL_GAIN, static_cast<ALfloat>(volume));
    }
    CHECK_AL_ERROR();
}

void AudioSource::setPitch(float pitch) {
    alSourcef(id, AL_PITCH, static_cast<ALfloat>(pitch));
    if (monoData) {
        alSourcef(monoId, AL_PITCH, static_cast<ALfloat>(pitch));
    }
    CHECK_AL_ERROR();
}

void AudioSource::setPosition(Position3d position) {
    if (!monoData) {
        return;
    }
    alSource3f(monoId, AL_POSITION, static_cast<ALfloat>(position.x),
               static_cast<ALfloat>(position.y),
               static_cast<ALfloat>(position.z));
    CHECK_AL_ERROR();
}

void AudioSource::setVelocity(Magnitude3d velocity) {
    if (!monoData) {
        return;
    }
    alSource3f(monoId, AL_VELOCITY, static_cast<ALfloat>(velocity.x),
               static_cast<ALfloat>(velocity.y),
               static_cast<ALfloat>(velocity.z));
    CHECK_AL_ERROR();
}

bool AudioSource::isPlaying() const {
    ALint state;
    if (this->isSpatialized && monoData) {
        alGetSourcei(monoId, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }
    alGetSourcei(id, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void AudioSource::playFrom(float seconds) {
    if (this->isSpatialized && monoData) {
        alSourcef(monoId, AL_SEC_OFFSET, static_cast<ALfloat>(seconds));
        CHECK_AL_ERROR();
        alSourcePlay(monoId);
        CHECK_AL_ERROR();
        return;
    }
    alSourcef(id, AL_SEC_OFFSET, static_cast<ALfloat>(seconds));
    CHECK_AL_ERROR();
    alSourcePlay(id);
    CHECK_AL_ERROR();
}

void AudioSource::useSpatialization() {
    ALint state;
    alGetSourcei(id, AL_SOURCE_STATE, &state);
    bool wasPlaying = (state == AL_PLAYING);

    ALfloat currentTime = 0.0f;
    if (wasPlaying) {
        alGetSourcef(id, AL_SEC_OFFSET, &currentTime);
        alSourceStop(id);
    }

    this->isSpatialized = true;
    if (monoData) {
        alSourcei(monoId, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(monoId, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcef(monoId, AL_REFERENCE_DISTANCE, 1.0f);
        alSourcef(monoId, AL_MAX_DISTANCE, 50.0f);
        CHECK_AL_ERROR();

        if (wasPlaying) {
            alSourcef(monoId, AL_SEC_OFFSET, currentTime);
            alSourcePlay(monoId);
            CHECK_AL_ERROR();
        }
    }
}

void AudioSource::disableSpatialization() {
    ALint state;
    alGetSourcei(monoId, AL_SOURCE_STATE, &state);
    bool wasPlaying = (state == AL_PLAYING);

    ALfloat currentTime = 0.0f;
    if (wasPlaying && monoData) {
        alGetSourcef(monoId, AL_SEC_OFFSET, &currentTime);
        alSourceStop(monoId);
    }

    this->isSpatialized = false;
    if (monoData) {
        alSourcei(monoId, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(monoId, AL_POSITION, 0.0f, 0.0f, 0.0f);
        CHECK_AL_ERROR();
    }

    if (wasPlaying) {
        alSourcef(id, AL_SEC_OFFSET, currentTime);
        alSourcePlay(id);
        CHECK_AL_ERROR();
    }
}

AudioSource::~AudioSource() {
    alDeleteSources(1, &id);
    alDeleteSources(1, &monoId);
}

Position3d AudioSource::getPosition() const {
    if (!monoData) {
        return Position3d{0.0f, 0.0f, 0.0f};
    }
    ALfloat x, y, z;
    alGetSource3f(monoId, AL_POSITION, &x, &y, &z);
    return Position3d{x, y, z};
}

Position3d AudioSource::getListenerPosition() const {
    Position3d pos;
    ALfloat x;
    ALfloat y;
    ALfloat z;
    alGetListener3f(AL_POSITION, &x, &y, &z);
    return Position3d{x, y, z};
};
