/*
 texture_create.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Basic texture creation and sampling functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/texture.h"
#include "atlas/units.h"
#include <glad/glad.h>

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
            } else {
                data[i + 0] = static_cast<unsigned char>(color2.r * 255);
                data[i + 1] = static_cast<unsigned char>(color2.g * 255);
                data[i + 2] = static_cast<unsigned char>(color2.b * 255);
            }
        }
    }

    Id textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    Texture::applyWrappingMode(params.wrappingModeS, GL_TEXTURE_WRAP_S);
    Texture::applyWrappingMode(params.wrappingModeT, GL_TEXTURE_WRAP_T);
    Texture::applyFilteringMode(params.minifyingFilter, true);
    Texture::applyFilteringMode(params.magnifyingFilter, false);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        if (borderColor.r < 0 || borderColor.r > 1 || borderColor.g < 0 ||
            borderColor.g > 1 || borderColor.b < 0 || borderColor.b > 1 ||
            borderColor.a < 0 || borderColor.a > 1) {
            throw std::runtime_error(
                "Border color values must be between 0 and 1");
        }
        float borderColorArr[4] = {static_cast<float>(borderColor.r),
                                   static_cast<float>(borderColor.g),
                                   static_cast<float>(borderColor.b),
                                   static_cast<float>(borderColor.a)};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                         borderColorArr);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    TextureCreationData creationData{width, height, 3};

    return Texture{Resource(), creationData, textureId, TextureType::Color,
                   borderColor};
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
            } else {
                finalColor = smallCheck ? color2 : color3;
            }

            data[i + 0] = static_cast<unsigned char>(finalColor.r * 255);
            data[i + 1] = static_cast<unsigned char>(finalColor.g * 255);
            data[i + 2] = static_cast<unsigned char>(finalColor.b * 255);
        }
    }

    Id textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    Texture::applyWrappingMode(params.wrappingModeS, GL_TEXTURE_WRAP_S);
    Texture::applyWrappingMode(params.wrappingModeT, GL_TEXTURE_WRAP_T);
    Texture::applyFilteringMode(params.minifyingFilter, true);
    Texture::applyFilteringMode(params.magnifyingFilter, false);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        float borderColorArr[4] = {static_cast<float>(borderColor.r),
                                   static_cast<float>(borderColor.g),
                                   static_cast<float>(borderColor.b),
                                   static_cast<float>(borderColor.a)};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                         borderColorArr);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    TextureCreationData creationData{width, height, 3};

    return Texture{Resource(), creationData, textureId, TextureType::Color,
                   borderColor};
}

Texture Texture::createTiledCheckerboard(int width, int height,
                                         const std::vector<CheckerTile> &tiles,
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
            int tileIndex = tileRow * cols + tileCol;
            if (tileIndex >= numTiles)
                tileIndex = numTiles - 1;

            const CheckerTile &tile = tiles[tileIndex];

            bool isWhite = ((x % tileWidth) / tile.checkSize % 2) ^
                           ((y % tileHeight) / tile.checkSize % 2);
            int i = (y * width + x) * 3;

            const Color &c = isWhite ? tile.color1 : tile.color2;
            data[i + 0] = static_cast<unsigned char>(c.r * 255);
            data[i + 1] = static_cast<unsigned char>(c.g * 255);
            data[i + 2] = static_cast<unsigned char>(c.b * 255);
        }
    }

    Id textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    Texture::applyWrappingMode(params.wrappingModeS, GL_TEXTURE_WRAP_S);
    Texture::applyWrappingMode(params.wrappingModeT, GL_TEXTURE_WRAP_T);
    Texture::applyFilteringMode(params.minifyingFilter, true);
    Texture::applyFilteringMode(params.magnifyingFilter, false);

    if (params.wrappingModeS == TextureWrappingMode::ClampToBorder ||
        params.wrappingModeT == TextureWrappingMode::ClampToBorder) {
        float borderColorArr[4] = {static_cast<float>(borderColor.r),
                                   static_cast<float>(borderColor.g),
                                   static_cast<float>(borderColor.b),
                                   static_cast<float>(borderColor.a)};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                         borderColorArr);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    TextureCreationData creationData{width, height, 3};
    return Texture{Resource(), creationData, textureId, TextureType::Color,
                   borderColor};
}
