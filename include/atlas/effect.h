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
enum class RenderTargetEffect { Invert = 0, Grayscale = 1, Sharpen = 2 };

class Effect {
  public:
    RenderTargetEffect type;
    Effect(RenderTargetEffect t) : type(t) {}
    virtual void applyToProgram(ShaderProgram &program) {};
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

#endif // ATLAS_EFFECT_H