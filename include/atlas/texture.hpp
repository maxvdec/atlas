/*
 texture.hpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture utilities and definitions for Atlas
 Copyright (c) 2025 maxvdec
*/

#ifndef ATLAS_TEXTURE_HPP
#define ATLAS_TEXTURE_HPP

#include "atlas/workspace.hpp"
#include <atlas/units.hpp>
#include <glad/glad.h>
#include <memory>
#include <vector>

enum class RepeatMode { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };
enum class FilteringMode { Nearest, Linear };
enum class MipmapFilteringMode {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureType {
    Color,
    Specular,
};

struct Texture {
    unsigned int ID;
    Size2d size;
    RepeatMode repeatMode = RepeatMode::Repeat;
    FilteringMode filteringMode = FilteringMode::Linear;
    MipmapFilteringMode mipmapFilteringMode =
        MipmapFilteringMode::LinearMipmapLinear;
    Color borderColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    Resource image = Resource();
    TextureType type = TextureType::Color;

    void setProperties();
    void fromImage(Resource resc, TextureType type);
};

struct CoreObject;

enum class EffectType { Inverse, Grayscale, Kernel, Blur, EdgeDetection };

struct Effect {
    EffectType type;
    float intensity = 1.0f;
};

struct RenderTarget;

typedef std::function<void(CoreObject *, RenderTarget *)> RenderingTargetFn;

struct RenderTarget {
    Texture texture;
    Size2d size;
    bool isOn = false;
    std::unique_ptr<CoreObject> fullScreenObject = nullptr;
    RenderingTargetFn dispatcher;
    bool isRendering = false;

    inline void enable() { isOn = true; }

    inline void disable() { isOn = false; }

    inline void addEffect(EffectType effect, float intensity = 1.0f) {
        effects.push_back({effect, intensity});
    }

    RenderTarget(Size2d size = Size2d(800, 600),
                 TextureType type = TextureType::Color);

    void renderToScreen();
    unsigned int fbo = 0;

    std::vector<Effect> effects;

  private:
    unsigned int rbo = 0;
    unsigned int texColorBuffer = 0;
};

#endif // ATLAS_TEXTURE_HPP
