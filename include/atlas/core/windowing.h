#ifndef ATLAS_WINDOWING_H
#define ATLAS_WINDOWING_H

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>

inline void atlasGetWindowSizeInPixels(SDL_Window *window, int *width,
                                       int *height) {
    int pixelWidth = 0;
    int pixelHeight = 0;
    if (window != nullptr &&
        !SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight)) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        float scale = SDL_GetWindowDisplayScale(window);
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
        pixelWidth = std::max(
            1, static_cast<int>(static_cast<float>(windowWidth) * scale));
        pixelHeight = std::max(
            1, static_cast<int>(static_cast<float>(windowHeight) * scale));
    }
    if (window != nullptr) {
        pixelWidth = std::max(1, pixelWidth);
        pixelHeight = std::max(1, pixelHeight);
    }
    if (width != nullptr) {
        *width = pixelWidth;
    }
    if (height != nullptr) {
        *height = pixelHeight;
    }
}

inline float atlasNormalizeAxisValue(std::int16_t value) {
    if (value >= 0) {
        return static_cast<float>(value) / 32767.0f;
    }
    return static_cast<float>(value) / 32768.0f;
}

inline float atlasGetTimeSeconds() {
    return static_cast<float>(SDL_GetTicksNS()) / 1'000'000'000.0f;
}

#endif
