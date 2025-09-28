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

#include "atlas/camera.h"
#include "atlas/component.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "finewave/audio.h"
#include "finewave/effect.h"
#include <memory>

class AudioPlayer : public Component {
  public:
    AudioPlayer() : sourceInitialized(false) {}

    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;

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

    inline void update(float dt) override {
        Window *window = Window::mainWindow;
        Camera *mainCamera = window->getCamera();

        if (mainCamera) {
            Position3d cameraPos = mainCamera->position;
            Normal3d cameraFront = mainCamera->getFrontVector();
            window->audioEngine->setListenerPosition(cameraPos);
            window->audioEngine->setListenerOrientation(
                cameraFront, Normal3d{0.0f, 1.0f, 0.0f});
            window->audioEngine->setListenerVelocity(mainCamera->getVelocity());
        }

        source->setPosition(this->object->getPosition());
    }

    inline void setPosition(Position3d position) {
        ensureSourceInitialized();
        source->setPosition(position);
    }

    inline void useSpatialization() {
        ensureSourceInitialized();
        source->useSpatialization();
    }

    inline void disableSpatialization() {
        ensureSourceInitialized();
        source->disableSpatialization();
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