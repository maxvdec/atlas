//
// clouds.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Clouds generation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/units.h"
#include <hydra/atmosphere.h>

Clouds::Clouds() {}

Id Clouds::getCloudTexture(int res) const {
    if (cachedTextureId != 0 && cachedResolution == res) {
        return cachedTextureId;
    }

    Id textureId = worleyNoise.getDetailTexture(res);
    cachedTextureId = textureId;
    cachedResolution = res;
    return cachedTextureId;
}
