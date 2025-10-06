//
// effect.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Effect definitions for post-processing
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef ATLAS_EFFECT_H
#define ATLAS_EFFECT_H

#include "atlas/core/shader.h"
#include <memory>
enum class RenderTargetEffect {
    Invert = 0,
    Grayscale = 1,
    Sharpen = 2,
    Blur = 3,
    EdgeDetection = 4,
    ColorCorrection = 5
};

class Effect {
  public:
    RenderTargetEffect type;
    Effect(RenderTargetEffect t) : type(t) {}
    virtual void applyToProgram(ShaderProgram &program, int index) {};
};

class Inversion : public Effect {
  public:
    Inversion() : Effect(RenderTargetEffect::Invert) {}
    static std::shared_ptr<Inversion> create() {
        return std::make_shared<Inversion>();
    }
};

class Grayscale : public Effect {
  public:
    Grayscale() : Effect(RenderTargetEffect::Grayscale) {}
    static std::shared_ptr<Grayscale> create() {
        return std::make_shared<Grayscale>();
    }
};

class Sharpen : public Effect {
  public:
    Sharpen() : Effect(RenderTargetEffect::Sharpen) {}
    static std::shared_ptr<Sharpen> create() {
        return std::make_shared<Sharpen>();
    }
};

class Blur : public Effect {
  public:
    float magnitude = 16.0;
    Blur(float magnitude = 16.0)
        : Effect(RenderTargetEffect::Blur), magnitude(magnitude) {}
    static std::shared_ptr<Blur> create(float magnitude = 16.0) {
        return std::make_shared<Blur>(magnitude);
    }
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             magnitude);
    }
};

class EdgeDetection : public Effect {
  public:
    EdgeDetection() : Effect(RenderTargetEffect::EdgeDetection) {}
    static std::shared_ptr<EdgeDetection> create() {
        return std::make_shared<EdgeDetection>();
    }
};

struct ColorCorrectionParameters {
    float exposure = 0.0;
    float contrast = 1.0;
    float saturation = 1.0;
    float gamma = 1.0;
    float temperature = 0.0;
    float tint = 0.0;
};

class ColorCorrection : public Effect {
  public:
    ColorCorrectionParameters params;

    ColorCorrection(ColorCorrectionParameters p = {})
        : Effect(RenderTargetEffect::ColorCorrection), params(p) {}
    static std::shared_ptr<ColorCorrection>
    create(ColorCorrectionParameters p = {}) {
        return std::make_shared<ColorCorrection>(p);
    }
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             params.exposure);
        program.setUniform1f("EffectFloat2[" + std::to_string((int)index) + "]",
                             params.contrast);
        program.setUniform1f("EffectFloat3[" + std::to_string((int)index) + "]",
                             params.saturation);
        program.setUniform1f("EffectFloat4[" + std::to_string((int)index) + "]",
                             params.gamma);
        program.setUniform1f("EffectFloat5[" + std::to_string((int)index) + "]",
                             params.temperature);
        program.setUniform1f("EffectFloat6[" + std::to_string((int)index) + "]",
                             params.tint);
    }
};

#endif // ATLAS_EFFECT_H