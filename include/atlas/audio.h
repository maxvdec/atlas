//
// audio.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Audio engine and related definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_AUDIO_H
#define ATLAS_AUDIO_H

#include "atlas/component.h"
#include "atlas/workspace.h"
#include "finewave/audio.h"
#include "finewave/effect.h"
#include <memory>

class AudioPlayer : public Component {
  public:
    AudioPlayer() : sourceInitialized(false) {}

    // Delete copy operations since unique_ptr is not copyable
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;

    // Add move operations
    AudioPlayer(AudioPlayer &&other) noexcept
        : source(std::move(other.source)),
          sourceInitialized(other.sourceInitialized) {
        other.sourceInitialized = false;
    }

    AudioPlayer &operator=(AudioPlayer &&other) noexcept {
        if (this != &other) {
            source = std::move(other.source);
            sourceInitialized = other.sourceInitialized;
            other.sourceInitialized = false;
        }
        return *this;
    }

    inline void init() override {
        if (!sourceInitialized) {
            source = std::make_unique<AudioSource>();
            sourceInitialized = true;
        }
    }

    inline void play() {
        ensureSourceInitialized();
        source->play();
    }

    inline void pause() {
        ensureSourceInitialized();
        source->pause();
    }

    inline void stop() {
        ensureSourceInitialized();
        source->stop();
    }

    inline void setSource(Resource sourceResource) {
        ensureSourceInitialized();
        source->fromFile(sourceResource);
    }

    std::unique_ptr<AudioSource> source;

  private:
    bool sourceInitialized;

    inline void ensureSourceInitialized() {
        if (!sourceInitialized) {
            source = std::make_unique<AudioSource>();
            sourceInitialized = true;
        }
    }
};

#endif // ATLAS_AUDIO_H