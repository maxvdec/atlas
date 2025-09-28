//
// audio.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Audio definitions and declarations
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef FINEWAVE_AUDIO_H
#define FINEWAVE_AUDIO_H

#include "atlas/units.h"
#include "atlas/workspace.h"
#include "finewave/effect.h"
#include <memory>
#include <string>

class AudioEngine {
  public:
    bool initialize();
    void shutdown();

    void setListenerPosition(Position3d position);
    void setListenerOrientation(Magnitude3d forward, Normal3d up);
    void setListenerVelocity(Magnitude3d velocity);
    void setMasterVolume(float volume);

    std::string deviceName;
};

class AudioData {
  public:
    static std::shared_ptr<AudioData> fromResource(Resource resource);
    ~AudioData();

    inline Id getId() const { return id; }

  private:
    Id id;
};

class AudioSource {
  public:
    AudioSource();
    ~AudioSource();

    void setData(std::shared_ptr<AudioData> buffer);
    void fromFile(Resource resource);

    void play();
    void pause();
    void stop();

    void setLooping(bool loop);
    void setVolume(float volume);
    void setPitch(float pitch);

    void setPosition(Position3d position);
    void setVelocity(Magnitude3d velocity);

    bool isPlaying() const;
    void playFrom(float seconds);

    void disableSpatialization();
    void applyEffect(const AudioEffect &effect);

    Position3d getPosition() const;
    Position3d getListenerPosition() const;

    void useSpatialization();

  private:
    Id id;
};

#endif // FINEWAVE_AUDIO_H