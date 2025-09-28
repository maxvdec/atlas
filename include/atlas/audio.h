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

class AudioPlayer : public Component {
  public:
    AudioPlayer() { source = AudioSource(); }
    AudioSource source;

    inline void init() override {}
    inline void play() { source.play(); }
    inline void pause() { source.pause(); }
    inline void stop() { source.stop(); }
    inline void setSource(Resource source) { this->source.fromFile(source); }
};

#endif // ATLAS_AUDIO_H