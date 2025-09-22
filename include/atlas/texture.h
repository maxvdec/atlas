/*
 texture.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture definition and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <array>
#pragma once

#include "atlas/core/renderable.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include <memory>
#include <string>

struct TextureCreationData {
    int width = 0;
    int height = 0;
    int channels = 0;
};

enum class TextureWrappingMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilteringMode { Nearest, Linear };

struct TextureParameters {
    TextureWrappingMode wrappingModeS = TextureWrappingMode::Repeat;
    TextureWrappingMode wrappingModeT = TextureWrappingMode::Repeat;
    TextureFilteringMode minifyingFilter = TextureFilteringMode::Linear;
    TextureFilteringMode magnifyingFilter = TextureFilteringMode::Linear;
};

enum class TextureType : int {
    Color = 0,
    Specular = 1,
    Cubemap = 2,
    Depth = 3,
    DepthCube = 4
};

struct CheckerTile {
    Color color1;
    Color color2;
    int checkSize;
};

struct Texture {
    Resource resource;
    TextureCreationData creationData;
    Id id;
    TextureType type;
    Color borderColor = {0, 0, 0, 0};

    static Texture fromResource(const Resource &resource,
                                TextureType type = TextureType::Color,
                                TextureParameters params = {},
                                Color borderColor = {0, 0, 0, 0});
    static Texture fromResourceName(const std::string &resourceName,
                                    TextureType type = TextureType::Color,
                                    TextureParameters params = {},
                                    Color borderColor = {0, 0, 0, 0});

    static Texture createCheckerboard(int width, int height, int checkSize,
                                      Color color1, Color color2,
                                      TextureParameters params = {},
                                      Color borderColor = {0, 0, 0, 0});

    static Texture createDoubleCheckerboard(int width, int height,
                                            int checkSizeBig,
                                            int checkSizeSmall, Color color1,
                                            Color color2, Color color3,
                                            TextureParameters params = {},
                                            Color borderColor = {0, 0, 0, 0});

    static Texture createTiledCheckerboard(
        int width, int height, const std::vector<CheckerTile> &tiles,
        TextureParameters params = {}, Color borderColor = {0, 0, 0, 0});

  private:
    static void applyWrappingMode(TextureWrappingMode mode, Id glAxis);
    static void applyFilteringMode(TextureFilteringMode mode, bool isMinifying);
};

struct Cubemap {
    std::array<Resource, 6> resources;
    TextureCreationData creationData;
    Id id;
    TextureType type = TextureType::Cubemap;

    static Cubemap fromResourceGroup(ResourceGroup &resourceGroup);
};

class Window;
class CoreObject;

enum class RenderTargetType { Scene, Multisampled, Shadow, CubeShadow };

class RenderTarget : public Renderable {
  public:
    RenderTarget() = default;

    Texture texture;
    RenderTargetType type;

    RenderTarget(Window &window,
                 RenderTargetType type = RenderTargetType::Scene,
                 int resolution = 1024);

    void display(Window &window, float zindex = 0);
    void hide();
    void show();

    std::shared_ptr<CoreObject> object = nullptr;

    void render() override;
    void resolve();

  private:
    Id fbo = 0;
    Id rbo = 0;
    Id resolveFbo = 0;

    friend class Window;
};

class Skybox : public Renderable {
  public:
    Skybox() = default;

    Cubemap cubemap;

    std::shared_ptr<CoreObject> object = nullptr;

    void display(Window &window);
    void hide();
    void show();

    void render() override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

  private:
    friend class Window;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
};

#endif // TEXTURE_H
