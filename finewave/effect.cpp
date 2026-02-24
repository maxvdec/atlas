//
// effect.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Effect implementations
// Copyright (c) 2025 Max Van den Eynde
//

#include "finewave/effect.h"
#include "finewave/audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

#define CHECK_EFFECTS()                                                        \
    ALCcontext *currentContext = alcGetCurrentContext();                       \
    ALCdevice *device = alcGetContextsDevice(currentContext);                  \
    if (!alcIsExtensionPresent(device, "ALC_EXT_EFX")) {                       \
        return;                                                                \
    }

namespace FWEFX {

static LPALGENEFFECTS alGenEffects = nullptr;
static LPALDELETEEFFECTS alDeleteEffects = nullptr;
static LPALISEFFECT alIsEffect = nullptr;
static LPALEFFECTI alEffecti = nullptr;
static LPALEFFECTF alEffectf = nullptr;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;

static void InitEFX() {
    if (alGenEffects)
        return;

    alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
    alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
    alIsEffect = (LPALISEFFECT)alGetProcAddress("alIsEffect");
    alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
    alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
    alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress(
        "alGenAuxiliaryEffectSlots");
    alDeleteAuxiliaryEffectSlots =
        (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress(
            "alDeleteAuxiliaryEffectSlots");
    alAuxiliaryEffectSloti =
        (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
}

} // namespace FWEFX

Reverb::Reverb() {
    CHECK_EFFECTS();
    FWEFX::InitEFX();
    FWEFX::alGenEffects(1, &id);
    FWEFX::alEffecti(id, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
}

void Reverb::setRoomSize(float size) {
    FWEFX::alEffectf(id, AL_REVERB_DECAY_TIME, size);
}
void Reverb::setDamping(float damping) {
    FWEFX::alEffectf(id, AL_REVERB_GAINHF, damping);
}
void Reverb::setWetLevel(float level) {
    FWEFX::alEffectf(id, AL_REVERB_LATE_REVERB_GAIN, level);
}
void Reverb::setDryLevel(float level) {
    FWEFX::alEffectf(id, AL_REVERB_GAIN, level);
}
void Reverb::setWidth(float width) {
    FWEFX::alEffectf(id, AL_REVERB_DIFFUSION, width);
}

Echo::Echo() {
    CHECK_EFFECTS();
    FWEFX::InitEFX();
    FWEFX::alGenEffects(1, &id);
    FWEFX::alEffecti(id, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
}

void Echo::setDelay(float delay) { FWEFX::alEffectf(id, AL_ECHO_DELAY, delay); }
void Echo::setDecay(float decay) {
    FWEFX::alEffectf(id, AL_ECHO_LRDELAY, decay);
}
void Echo::setWetLevel(float level) {
    FWEFX::alEffectf(id, AL_ECHO_FEEDBACK, level);
}
void Echo::setDryLevel(float level) {
    FWEFX::alEffectf(id, AL_ECHO_MAX_FEEDBACK, level);
}

Distortion::Distortion() {
    CHECK_EFFECTS();
    FWEFX::InitEFX();
    FWEFX::alGenEffects(1, &id);
    FWEFX::alEffecti(id, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
}

void Distortion::setEdge(float edge) {
    FWEFX::alEffectf(id, AL_DISTORTION_EDGE, edge);
}
void Distortion::setGain(float gain) {
    FWEFX::alEffectf(id, AL_DISTORTION_GAIN, gain);
}
void Distortion::setLowpassCutoff(float cutoff) {
    FWEFX::alEffectf(id, AL_DISTORTION_LOWPASS_CUTOFF, cutoff);
}

void AudioSource::applyEffect(const AudioEffect &effect) const {
    CHECK_EFFECTS();
    FWEFX::alAuxiliaryEffectSloti(0, AL_EFFECTSLOT_EFFECT, effect.id);
    alSource3i(id, AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL);
}