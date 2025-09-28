//
// effect.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Audio effect definitions
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef FINEWAVE_EFFECT_H
#define FINEWAVE_EFFECT_H

#include "atlas/units.h"
class AudioEffect {
  protected:
    Id id;

    friend class AudioSource;
};

class Reverb : public AudioEffect {
  public:
    Reverb();
    void setRoomSize(float size);
    void setDamping(float damping);
    void setWetLevel(float level);
    void setDryLevel(float level);
    void setWidth(float width);
};

class Echo : public AudioEffect {
  public:
    Echo();
    void setDelay(float delay);
    void setDecay(float decay);
    void setWetLevel(float level);
    void setDryLevel(float level);
};

class Distortion : public AudioEffect {
  public:
    Distortion();
    void setEdge(float edge);
    void setGain(float gain);
    void setLowpassCutoff(float cutoff);
};

#endif // FINEWAVE_EFFECT_H