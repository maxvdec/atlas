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

/**
 * @brief Abstract base class for all audio effects that can be applied to
 * AudioSource instances.
 *
 */
class AudioEffect {
  protected:
    Id id;

    friend class AudioSource;
};

/**
 * @brief Reverb audio effect that simulates acoustic reflections in a space.
 *
 * \subsection reverb-example Example
 * ```cpp
 * // Create reverb effect for a cathedral environment
 * Reverb cathedralReverb;
 * cathedralReverb.setRoomSize(0.9f);    // Large room
 * cathedralReverb.setDamping(0.3f);     // Low damping for long reflections
 * cathedralReverb.setWetLevel(0.6f);    // 60% reverb
 * cathedralReverb.setDryLevel(0.4f);    // 40% original signal
 * cathedralReverb.setWidth(1.0f);       // Full stereo width
 *
 * // Apply to audio source
 * AudioSource footsteps;
 * footsteps.applyEffect(cathedralReverb);
 * ```
 *
 */
class Reverb : public AudioEffect {
  public:
    /**
     * @brief Constructs a new Reverb effect.
     *
     */
    Reverb();
    /**
     * @brief Sets the room size parameter for the reverb.
     *
     * @param size Room size value (0.0 to 1.0).
     */
    void setRoomSize(float size);
    /**
     * @brief Sets the damping parameter for the reverb.
     *
     * @param damping Damping value (0.0 to 1.0).
     */
    void setDamping(float damping);
    /**
     * @brief Sets the wet level (reverb amount) for the effect.
     *
     * @param level Wet level value (0.0 to 1.0).
     */
    void setWetLevel(float level);
    /**
     * @brief Sets the dry level (original signal amount) for the effect.
     *
     * @param level Dry level value (0.0 to 1.0).
     */
    void setDryLevel(float level);
    /**
     * @brief Sets the stereo width of the reverb.
     *
     * @param width Width value (0.0 to 1.0).
     */
    void setWidth(float width);
};

/**
 * @brief Echo audio effect that creates delayed repetitions of the original
 * sound.
 *
 * \subsection echo-example Example
 * ```cpp
 * // Create echo effect with 500ms delay
 * Echo canyonEcho;
 * canyonEcho.setDelay(0.5f);        // 500ms delay
 * canyonEcho.setDecay(0.4f);        // 40% decay per repetition
 * canyonEcho.setWetLevel(0.5f);     // 50% echo
 * canyonEcho.setDryLevel(0.7f);     // 70% original signal
 *
 * // Apply to voice audio
 * AudioSource voiceLine;
 * voiceLine.applyEffect(canyonEcho);
 * ```
 *
 */
class Echo : public AudioEffect {
  public:
    /**
     * @brief Constructs a new Echo effect.
     *
     */
    Echo();
    /**
     * @brief Sets the delay time for the echo.
     *
     * @param delay Delay time in seconds.
     */
    void setDelay(float delay);
    /**
     * @brief Sets the decay rate for the echo.
     *
     * @param decay Decay value (0.0 to 1.0).
     */
    void setDecay(float decay);
    /**
     * @brief Sets the wet level (echo amount) for the effect.
     *
     * @param level Wet level value (0.0 to 1.0).
     */
    void setWetLevel(float level);
    /**
     * @brief Sets the dry level (original signal amount) for the effect.
     *
     * @param level Dry level value (0.0 to 1.0).
     */
    void setDryLevel(float level);
};

/**
 * @brief Distortion audio effect that adds harmonic distortion to the sound.
 *
 * \subsection distortion-example Example
 * ```cpp
 * // Create distortion effect for electric guitar sound
 * Distortion guitarDistortion;
 * guitarDistortion.setEdge(0.7f);           // High edge for crunch
 * guitarDistortion.setGain(3.0f);           // 3x gain multiplier
 * guitarDistortion.setLowpassCutoff(4000.0f); // 4kHz cutoff
 *
 * // Apply to guitar audio source
 * AudioSource guitarSound;
 * guitarSound.applyEffect(guitarDistortion);
 * ```
 *
 */
class Distortion : public AudioEffect {
  public:
    /**
     * @brief Constructs a new Distortion effect.
     *
     */
    Distortion();
    /**
     * @brief Sets the edge parameter for the distortion.
     *
     * @param edge Edge sharpness value.
     */
    void setEdge(float edge);
    /**
     * @brief Sets the gain for the distortion.
     *
     * @param gain Gain multiplier value.
     */
    void setGain(float gain);
    /**
     * @brief Sets the lowpass filter cutoff frequency.
     *
     * @param cutoff Cutoff frequency in Hz.
     */
    void setLowpassCutoff(float cutoff);
};

#endif // FINEWAVE_EFFECT_H