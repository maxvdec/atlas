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

/**
 * @brief Component that provides audio playback capabilities to an object. It
 * can be attach to any object in the scene and allows for playing, pausing and
 * stopping audio.
 *
 * \note Finewave will be required for this component to work. This component
 * under the hood uses Finewave's AudioSource class to manage audio playback.
 *
 * \subsection audio-player-example Example
 * ```cpp
 * // Create an AudioPlayer component
 * auto audioPlayer = std::make_shared<AudioPlayer>();
 * // Initialize the audio player
 * audioPlayer->init();
 * // Set the audio source from a file
 * audioPlayer->setSource(Workspace::get().getResource("AudioResource"));
 * // Play the audio
 * audioPlayer->play();
 * ```
 */
class AudioPlayer : public Component {
  public:
    /**
     * @brief Construct a new empty AudioPlayer object
     *
     */
    AudioPlayer() : sourceInitialized(false) {}

    /**
     * @brief Destroy the Audio Player object and release resources.
     *
     */
    AudioPlayer(const AudioPlayer &) = delete;
    /**
     * @brief Destroy the Audio Player object and release resources.
     *
     * @return AudioPlayer&
     */
    AudioPlayer &operator=(const AudioPlayer &) = delete;

    /**
     * @brief Move constructor for AudioPlayer.
     *
     * @param other The AudioPlayer instance to move from.
     */
    AudioPlayer(AudioPlayer &&other) noexcept
        : source(std::move(other.source)),
          sourceInitialized(other.sourceInitialized) {
        other.sourceInitialized = false;
    }

    /**
     * @brief Move assignment operator for AudioPlayer.
     *
     * @param other The AudioPlayer instance to move from.
     * @return AudioPlayer& Reference to the current instance after move.
     */
    AudioPlayer &operator=(AudioPlayer &&other) noexcept {
        if (this != &other) {
            source = std::move(other.source);
            sourceInitialized = other.sourceInitialized;
            other.sourceInitialized = false;
        }
        return *this;
    }

    /**
     * @brief Initialize the audio player and prepare it for playback.
     *
     */
    inline void init() override {
        if (!sourceInitialized) {
            source = std::make_unique<AudioSource>();
            sourceInitialized = true;
        }
    }

    /**
     * @brief Play the audio from the beginning or resume if paused.
     *
     */
    inline void play() {
        ensureSourceInitialized();
        source->play();
    }

    /**
     * @brief Pause the audio playback. It can be resumed later.
     *
     */
    inline void pause() {
        ensureSourceInitialized();
        source->pause();
    }

    /**
     * @brief Stop the audio playback and reset to the beginning.
     *
     */
    inline void stop() {
        ensureSourceInitialized();
        source->stop();
    }

    /**
     * @brief Set the source from where the audio will be played.
     *
     * @param sourceResource The resource containing the audio file.
     */
    inline void setSource(Resource sourceResource) {
        ensureSourceInitialized();
        source->fromFile(sourceResource);
    }

    /**
     * @brief Update the audio player state. This method is called every frame.
     *
     * @param dt The delta time since the last update.
     */
    inline void update(float) override {
        ensureSourceInitialized();
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

    /**
     * @brief Set the position of the audio source in 3D space. \warning This is
     * used when spatialization is on.
     *
     * @param position The new position of the audio source.
     */
    inline void setPosition(Position3d position) {
        ensureSourceInitialized();
        source->setPosition(position);
    }

    /**
     * @brief Enable spatialization for the audio source, making it 3D and
     * affected by the listener's position.
     *
     */
    inline void useSpatialization() {
        ensureSourceInitialized();
        source->useSpatialization();
    }

    /**
     * @brief Disable spatialization for the audio source, making it play
     * uniformly regardless of the listener's position.
     *
     */
    inline void disableSpatialization() {
        ensureSourceInitialized();
        source->disableSpatialization();
    }

    /**
     * @brief Get the audio source. This allows for advanced manipulation of the
     * audio playback.
     *
     */
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