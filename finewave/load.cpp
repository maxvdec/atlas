//
// load.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Function to load and parse audio files
// Copyright (c) 2025 Max Van den Eynde
//

#include "finewave/audio.h"
#include "atlas/workspace.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>

struct WavHeader {
    char riff[4]; // "RIFF"
    unsigned int chunkSize;
    char wave[4];               // "WAVE"
    char fmt[4];                // "fmt "
    unsigned int subchunk1Size; // 16 for PCM
    unsigned short audioFormat; // 1 = PCM
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    char data[4];          // "data"
    unsigned int dataSize; // size of raw PCM data
};

const char *getALErrorString(ALenum error) {
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
            std::cerr << "OpenAL error: " << getALErrorString(err) << " ("     \
                      << err << ") at " << __FILE__ << ":" << __LINE__         \
                      << std::endl;                                            \
        }                                                                      \
    }

std::shared_ptr<AudioData> AudioData::fromResource(Resource resource) {
    CHECK_AL_ERROR();
    if (resource.type != ResourceType::Audio) {
        throw std::invalid_argument("Resource is not of type Audio");
    }

    std::ifstream file(resource.path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open audio file: " +
                                 resource.path.string());
    }

    WavHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(WavHeader));

    if (std::string(header.riff, 4) != "RIFF" ||
        std::string(header.wave, 4) != "WAVE") {
        throw std::runtime_error("Invalid WAV file format: " +
                                 resource.path.string());
    }

    std::vector<char> data(header.dataSize);
    file.read(data.data(), header.dataSize);

    ALenum format;
    if (header.numChannels == 1) {
        format =
            (header.bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    } else if (header.numChannels == 2) {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_STEREO8
                                             : AL_FORMAT_STEREO16;
    } else {
        throw std::runtime_error("Unsupported number of channels: " +
                                 std::to_string(header.numChannels));
    }

    ALuint buffer;
    alGenBuffers(1, &buffer);
    CHECK_AL_ERROR();

    if (!alIsBuffer(buffer)) {
        std::cerr << "Failed to generate OpenAL buffer" << std::endl;
        throw std::runtime_error("Failed to generate OpenAL buffer");
    }

    alBufferData(buffer, format, data.data(), header.dataSize,
                 header.sampleRate);
    CHECK_AL_ERROR();

    auto audioData = std::make_shared<AudioData>();
    audioData->id = buffer;
    return audioData;
}

AudioData::~AudioData() {
    if (alIsBuffer(id)) {
        alDeleteBuffers(1, &id);
    }
}