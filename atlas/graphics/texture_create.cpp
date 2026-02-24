/*
 texture_create.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Basic texture creation and sampling functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.h"
#include "atlas/tracer/log.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <algorithm>
#include <cmath>
#include <memory>

Texture Texture::createCheckerboard(int width, int height, int checkSize,
                                    Color color1, Color color2,
                                    TextureParameters params,
                                    Color borderColor) {
    std::vector<unsigned char> data(width * height * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 3;

            bool isWhite = ((x / checkSize) % 2) ^ ((y / checkSize) % 2);

            if (isWhite) {
                data[i + 0] = static_cast<unsigned char>(color1.r * 255);
                data[i + 1] = static_cast<unsigned char>(color1.g * 255);
                data[i + 2] = static_cast<unsigned char>(color1.b * 255);
            }
            else {
                data[i + 0] = static_cast<unsigned char>(color2.r * 255);
                data[i + 1] = static_cast<unsigned char>(color2.g * 255);
                data[i + 2] = static_cast<unsigned char>(color2.b * 255);
            }
        }
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::sRgb8, width, height,
        opal::TextureDataFormat::Rgb, data.data(), 1);

    Texture::applyWrappingMode(params.wrappingModeS, opal::TextureAxis::S,
                               opalTexture);
    Texture::applyWrappingMode(params.wrappingModeT, opal::TextureAxis::T,
                               opalTexture);
    Texture::applyFilteringModes(params.minifyingFilter,
                                 params.magnifyingFilter, opalTexture);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            atlas_warning("Border colors must be between 0 and 1");
        }
        opalTexture->changeBorderColor(borderColor.toGlm());
    }

    opalTexture->automaticallyGenerateMipmaps();

    TextureCreationData creationData{.width = width, .height = height, .channels = 3};

    return Texture{
        .resource = Resource(), .creationData = creationData, .id = opalTexture->textureID,
        .texture = opalTexture, .type = TextureType::Color, .borderColor = borderColor
    };
}

Texture Texture::createDoubleCheckerboard(
    int width, int height, int checkSizeBig, int checkSizeSmall, Color color1,
    Color color2, Color color3, TextureParameters params, Color borderColor) {
    std::vector<unsigned char> data(width * height * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 3;

            bool bigCheck = ((x / checkSizeBig) % 2) ^ ((y / checkSizeBig) % 2);

            bool smallCheck =
                ((x / checkSizeSmall) % 2) ^ ((y / checkSizeSmall) % 2);

            Color finalColor;

            if (bigCheck) {
                finalColor = smallCheck ? color1 : color3;
            }
            else {
                finalColor = smallCheck ? color2 : color3;
            }

            data[i + 0] = static_cast<unsigned char>(finalColor.r * 255);
            data[i + 1] = static_cast<unsigned char>(finalColor.g * 255);
            data[i + 2] = static_cast<unsigned char>(finalColor.b * 255);
        }
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::sRgb8, width, height,
        opal::TextureDataFormat::Rgb, data.data(), 1);

    Texture::applyWrappingMode(params.wrappingModeS, opal::TextureAxis::S,
                               opalTexture);
    Texture::applyWrappingMode(params.wrappingModeT, opal::TextureAxis::T,
                               opalTexture);
    Texture::applyFilteringModes(params.minifyingFilter,
                                 params.magnifyingFilter, opalTexture);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            atlas_warning("Border colors must be between 0 and 1");
        }
        opalTexture->changeBorderColor(borderColor.toGlm());
    }

    opalTexture->automaticallyGenerateMipmaps();

    TextureCreationData creationData{.width = width, .height = height, .channels = 3};

    return Texture{
        .resource = Resource(),
        .creationData = creationData,
        .id = opalTexture->textureID,
        .texture = opalTexture,
        .type = TextureType::Color,
        .borderColor = borderColor
    };
}

Texture Texture::createTiledCheckerboard(int width, int height,
                                         const std::vector<CheckerTile>& tiles,
                                         TextureParameters params,
                                         Color borderColor) {
    std::vector<unsigned char> data(width * height * 3);

    int numTiles = static_cast<int>(tiles.size());
    int rows = static_cast<int>(std::sqrt(numTiles));
    int cols = (numTiles + rows - 1) / rows;

    int tileWidth = width / cols;
    int tileHeight = height / rows;

    for (int y = 0; y < height; ++y) {
        int tileRow = y / tileHeight;
        for (int x = 0; x < width; ++x) {
            int tileCol = x / tileWidth;
            int tileIndex = (tileRow * cols) + tileCol;
            if (tileIndex >= numTiles)
                tileIndex = numTiles - 1;

            const CheckerTile& tile = tiles[tileIndex];

            bool isWhite = ((x % tileWidth) / tile.checkSize % 2) ^
                ((y % tileHeight) / tile.checkSize % 2);
            int i = (y * width + x) * 3;

            const Color& c = isWhite ? tile.color1 : tile.color2;
            data[i + 0] = static_cast<unsigned char>(c.r * 255);
            data[i + 1] = static_cast<unsigned char>(c.g * 255);
            data[i + 2] = static_cast<unsigned char>(c.b * 255);
        }
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::sRgb8, width, height,
        opal::TextureDataFormat::Rgb, data.data(), 1);

    Texture::applyWrappingMode(params.wrappingModeS, opal::TextureAxis::S,
                               opalTexture);
    Texture::applyWrappingMode(params.wrappingModeT, opal::TextureAxis::T,
                               opalTexture);
    Texture::applyFilteringModes(params.minifyingFilter,
                                 params.magnifyingFilter, opalTexture);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            atlas_warning("Border colors must be between 0 and 1");
        }
        opalTexture->changeBorderColor(borderColor.toGlm());
    }

    opalTexture->automaticallyGenerateMipmaps();

    TextureCreationData creationData{
        .width = width, .height = height, .channels = 3
    };
    return Texture{
        .resource = Resource(),
        .creationData = creationData,
        .id = opalTexture->textureID,
        .texture = opalTexture,
        .type = TextureType::Color,
        .borderColor = borderColor
    };
}

Texture Texture::createRainStreak(int width, int height,
                                  TextureParameters params, Color borderColor) {
    if (width <= 0 || height <= 0) {
        atlas_error("Rain streak texture dimensions must be positive");
    }

    std::vector<unsigned char> data(width * height * 4, 0);

    float center = (static_cast<float>(width) - 1.0f) * 0.5f;
    float invHalfWidth = 1.0f / std::max(1.0f, center);

    for (int y = 0; y < height; ++y) {
        float v = static_cast<float>(y) / static_cast<float>(height - 1);
        float taper = 1.0f - v;
        float headGlow = std::exp(-v * 6.0f);

        for (int x = 0; x < width; ++x) {
            float offset = (static_cast<float>(x) - center) * invHalfWidth;
            float radial = std::exp(-offset * offset * 12.0f);
            float alpha = std::clamp(
                (radial * (0.25f + taper * 0.65f)) + (headGlow * 0.1f), 0.0f, 1.0f);

            float brightness =
                std::clamp((radial * 0.8f) + (headGlow * 0.2f), 0.0f, 1.0f);
            float tint = 0.65f + (0.35f * headGlow);

            int index = (y * width + x) * 4;
            data[index + 0] = static_cast<unsigned char>(brightness * 180.0f);
            data[index + 1] = static_cast<unsigned char>(brightness * 200.0f);
            data[index + 2] = static_cast<unsigned char>(
                std::clamp(tint * 255.0f, 0.0f, 255.0f));
            data[index + 3] = static_cast<unsigned char>(alpha * 255.0f);
        }
    }

    auto opalTexture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::sRgba8, width,
        height, opal::TextureDataFormat::Rgba, data.data(), 1);

    Texture::applyWrappingMode(params.wrappingModeS, opal::TextureAxis::S,
                               opalTexture);
    Texture::applyWrappingMode(params.wrappingModeT, opal::TextureAxis::T,
                               opalTexture);
    Texture::applyFilteringModes(params.minifyingFilter,
                                 params.magnifyingFilter, opalTexture);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            atlas_warning("Border colors values must be between 0 and 1");
        }
        opalTexture->changeBorderColor(borderColor.toGlm());
    }

    opalTexture->automaticallyGenerateMipmaps();

    TextureCreationData creationData{
        .width = width, .height = height, .channels = 4
    };
    return Texture{
        .resource = Resource(),
        .creationData = creationData,
        .id = opalTexture->textureID,
        .texture = opalTexture,
        .type = TextureType::Color,
        .borderColor = borderColor
    };
}
