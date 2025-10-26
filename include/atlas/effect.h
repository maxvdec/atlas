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
#include "atlas/units.h"
#include <iostream>
#include <memory>

/**
 * @brief Enumeration of available post-processing effects that can be applied
 * to render targets.
 *
 */
enum class RenderTargetEffect {
    /**
     * @brief Inverts the colors of the rendered image.
     *
     */
    Invert = 0,
    /**
     * @brief Converts the rendered image to grayscale.
     *
     */
    Grayscale = 1,
    /**
     * @brief Applies a sharpening filter to enhance edges.
     *
     */
    Sharpen = 2,
    /**
     * @brief Applies a blur effect to the rendered image.
     *
     */
    Blur = 3,
    /**
     * @brief Detects and highlights edges in the rendered image.
     *
     */
    EdgeDetection = 4,
    /**
     * @brief Applies color correction adjustments like exposure and contrast.
     *
     */
    ColorCorrection = 5,
    MotionBlur = 6,
    ChromaticAberration = 7,
    Posterization = 8,
    Pixelation = 9,
    Dilation = 10,
};

/**
 * @brief Base class for all post-processing effects that can be applied to a
 * render target. Effects are applied via shader uniforms.
 *
 */
class Effect {
  public:
    /**
     * @brief The type of effect this instance represents.
     *
     */
    RenderTargetEffect type;
    /**
     * @brief Constructs a new Effect object.
     *
     * @param t The type of effect.
     */
    Effect(RenderTargetEffect t) : type(t) {}
    /**
     * @brief Applies the effect's parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    virtual void applyToProgram(ShaderProgram &program, int index) {};
};

/**
 * @brief Post-processing effect that inverts all colors in the rendered image.
 * White becomes black, red becomes cyan, etc.
 *
 */
class Inversion : public Effect {
  public:
    /**
     * @brief Constructs a new Inversion effect.
     *
     */
    Inversion() : Effect(RenderTargetEffect::Invert) {}
    /**
     * @brief Creates a shared pointer to an Inversion effect.
     *
     * @return (std::shared_ptr<Inversion>) The created inversion effect.
     */
    static std::shared_ptr<Inversion> create() {
        return std::make_shared<Inversion>();
    }
};

/**
 * @brief Post-processing effect that converts the rendered image to grayscale
 * by calculating luminance.
 *
 */
class Grayscale : public Effect {
  public:
    /**
     * @brief Constructs a new Grayscale effect.
     *
     */
    Grayscale() : Effect(RenderTargetEffect::Grayscale) {}
    /**
     * @brief Creates a shared pointer to a Grayscale effect.
     *
     * @return (std::shared_ptr<Grayscale>) The created grayscale effect.
     */
    static std::shared_ptr<Grayscale> create() {
        return std::make_shared<Grayscale>();
    }
};

/**
 * @brief Post-processing effect that applies a sharpening kernel to enhance
 * edges and details in the rendered image.
 *
 */
class Sharpen : public Effect {
  public:
    /**
     * @brief Constructs a new Sharpen effect.
     *
     */
    Sharpen() : Effect(RenderTargetEffect::Sharpen) {}
    /**
     * @brief Creates a shared pointer to a Sharpen effect.
     *
     * @return (std::shared_ptr<Sharpen>) The created sharpen effect.
     */
    static std::shared_ptr<Sharpen> create() {
        return std::make_shared<Sharpen>();
    }
};

/**
 * @brief Post-processing effect that applies a Gaussian blur to the rendered
 * image. The blur magnitude can be controlled.
 *
 */
class Blur : public Effect {
  public:
    /**
     * @brief The magnitude (radius) of the blur effect.
     *
     */
    float magnitude = 16.0;
    /**
     * @brief Constructs a new Blur effect.
     *
     * @param magnitude The blur radius. Higher values create stronger blur.
     */
    Blur(float magnitude = 16.0)
        : Effect(RenderTargetEffect::Blur), magnitude(magnitude) {}
    /**
     * @brief Creates a shared pointer to a Blur effect.
     *
     * @param magnitude The blur radius.
     * @return (std::shared_ptr<Blur>) The created blur effect.
     */
    static std::shared_ptr<Blur> create(float magnitude = 16.0) {
        return std::make_shared<Blur>(magnitude);
    }
    /**
     * @brief Applies the blur magnitude to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             magnitude);
    }
};

/**
 * @brief Post-processing effect that detects and highlights edges in the
 * rendered image using an edge detection kernel.
 *
 */
class EdgeDetection : public Effect {
  public:
    /**
     * @brief Constructs a new EdgeDetection effect.
     *
     */
    EdgeDetection() : Effect(RenderTargetEffect::EdgeDetection) {}
    /**
     * @brief Creates a shared pointer to an EdgeDetection effect.
     *
     * @return (std::shared_ptr<EdgeDetection>) The created edge detection
     * effect.
     */
    static std::shared_ptr<EdgeDetection> create() {
        return std::make_shared<EdgeDetection>();
    }
};

/**
 * @brief Structure containing parameters for color correction post-processing.
 * Allows fine-tuning of exposure, contrast, saturation, gamma, temperature,
 * and tint.
 *
 */
struct ColorCorrectionParameters {
    /**
     * @brief Exposure adjustment. Positive values brighten, negative values
     * darken.
     *
     */
    float exposure = 0.0;
    /**
     * @brief Contrast adjustment. Values > 1.0 increase contrast, < 1.0
     * decrease it.
     *
     */
    float contrast = 1.0;
    /**
     * @brief Saturation adjustment. 1.0 is normal, 0.0 is grayscale, > 1.0
     * increases saturation.
     *
     */
    float saturation = 1.0;
    /**
     * @brief Gamma correction value. Typically around 1.0 to 2.2.
     *
     */
    float gamma = 1.0;
    /**
     * @brief Temperature adjustment. Positive values add warmth (red),
     * negative values add coolness (blue).
     *
     */
    float temperature = 0.0;
    /**
     * @brief Tint adjustment. Positive values add green, negative values add
     * magenta.
     *
     */
    float tint = 0.0;
};

/**
 * @brief Post-processing effect that applies comprehensive color correction to
 * the rendered image, including exposure, contrast, saturation, and color
 * temperature adjustments.
 *
 * \subsection color-correction-example Example
 * ```cpp
 * // Create a render target
 * RenderTarget renderTarget(window);
 * // Define color correction parameters
 * ColorCorrectionParameters params;
 * params.exposure = 0.5f;      // Increase brightness
 * params.contrast = 1.2f;      // Increase contrast slightly
 * params.saturation = 1.1f;    // Boost saturation
 * params.temperature = 0.1f;   // Add warmth
 * // Create and add the color correction effect
 * auto colorCorrection = ColorCorrection::create(params);
 * renderTarget.addEffect(colorCorrection);
 * ```
 *
 */
class ColorCorrection : public Effect {
  public:
    /**
     * @brief The color correction parameters to apply.
     *
     */
    ColorCorrectionParameters params;

    /**
     * @brief Constructs a new ColorCorrection effect.
     *
     * @param p The color correction parameters to use.
     */
    ColorCorrection(ColorCorrectionParameters p = {})
        : Effect(RenderTargetEffect::ColorCorrection), params(p) {}
    /**
     * @brief Creates a shared pointer to a ColorCorrection effect.
     *
     * @param p The color correction parameters to use.
     * @return (std::shared_ptr<ColorCorrection>) The created color correction
     * effect.
     */
    static std::shared_ptr<ColorCorrection>
    create(ColorCorrectionParameters p = {}) {
        return std::make_shared<ColorCorrection>(p);
    }
    /**
     * @brief Applies all color correction parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
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

struct MotionBlurParameters {
    int size = 8;
    float separation = 1.0;
};

class MotionBlur : public Effect {
  public:
    MotionBlur(MotionBlurParameters p = {})
        : Effect(RenderTargetEffect::MotionBlur), params(p) {}
    /**
     * @brief Creates a shared pointer to a MotionBlur effect.
     *
     * @param p The motion blur parameters to use.
     * @return (std::shared_ptr<MotionBlur>) The created motion blur effect.
     */
    static std::shared_ptr<MotionBlur> create(MotionBlurParameters p = {}) {
        return std::make_shared<MotionBlur>(p);
    }
    MotionBlurParameters params;
    /**
     * @brief Applies all motion blur parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             params.size);
        program.setUniform1f("EffectFloat2[" + std::to_string((int)index) + "]",
                             params.separation);
    }
};

struct ChromaticAberrationParameters {
    float red = 0.01;
    float green = 0.006;
    float blue = -0.006;
    Magnitude2d direction;
};

class ChromaticAberration : public Effect {
  public:
    ChromaticAberration(ChromaticAberrationParameters p = {})
        : Effect(RenderTargetEffect::ChromaticAberration), params(p) {}
    /**
     * @brief Creates a shared pointer to a ChromaticAberration effect.
     *
     * @param p The chromatic aberration parameters to use.
     * @return (std::shared_ptr<ChromaticAberration>) The created chromatic
     * aberration effect.
     */
    static std::shared_ptr<ChromaticAberration>
    create(ChromaticAberrationParameters p = {}) {
        return std::make_shared<ChromaticAberration>(p);
    }
    ChromaticAberrationParameters params;
    /**
     * @brief Applies all chromatic aberration parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             params.red);
        program.setUniform1f("EffectFloat2[" + std::to_string((int)index) + "]",
                             params.green);
        program.setUniform1f("EffectFloat3[" + std::to_string((int)index) + "]",
                             params.blue);
        program.setUniform1f("EffectFloat4[" + std::to_string((int)index) + "]",
                             params.direction.x);
        program.setUniform1f("EffectFloat5[" + std::to_string((int)index) + "]",
                             params.direction.y);
    }
};

struct PosterizationParameters {
    float levels = 5.0f;
};

class Posterization : public Effect {
  public:
    Posterization(PosterizationParameters p = {})
        : Effect(RenderTargetEffect::Posterization), params(p) {}
    /**
     * @brief Creates a shared pointer to a Posterization effect.
     *
     * @param p The posterization parameters to use.
     * @return (std::shared_ptr<Posterization>) The created posterization
     * effect.
     */
    static std::shared_ptr<Posterization>
    create(PosterizationParameters p = {}) {
        return std::make_shared<Posterization>(p);
    }
    PosterizationParameters params;
    /**
     * @brief Applies all posterization parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             params.levels);
    }
};

struct PixelationParameters {
    int pixelSize = 5;
};

class Pixelation : public Effect {
  public:
    Pixelation(PixelationParameters p = {})
        : Effect(RenderTargetEffect::Pixelation), params(p) {}
    /**
     * @brief Creates a shared pointer to a Pixelation effect.
     *
     * @param p The pixelation parameters to use.
     * @return (std::shared_ptr<Pixelation>) The created pixelation
     * effect.
     */
    static std::shared_ptr<Pixelation> create(PixelationParameters p = {}) {
        return std::make_shared<Pixelation>(p);
    }
    PixelationParameters params;
    /**
     * @brief Applies all pixelation parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             (float)params.pixelSize);
    }
};

struct DilationParameters {
    int size = 5.0;
    float separation = 2.0;
};

class Dilation : public Effect {
  public:
    Dilation(DilationParameters p = {})
        : Effect(RenderTargetEffect::Dilation), params(p) {}
    /**
     * @brief Creates a shared pointer to a Dialation effect.
     *
     * @param p The dialation parameters to use.
     * @return (std::shared_ptr<Dialation>) The created dialation
     * effect.
     */
    static std::shared_ptr<Dilation> create(DilationParameters p = {}) {
        return std::make_shared<Dilation>(p);
    }
    DilationParameters params;
    /**
     * @brief Applies all dialation parameters to the shader program.
     *
     * @param program The shader program to apply the effect to.
     * @param index The index of the effect in the effect array.
     */
    void applyToProgram(ShaderProgram &program, int index) override {
        program.setUniform1f("EffectFloat1[" + std::to_string((int)index) + "]",
                             (float)params.size);
        program.setUniform1f("EffectFloat2[" + std::to_string((int)index) + "]",
                             params.separation);
    }
};

#endif // ATLAS_EFFECT_H
