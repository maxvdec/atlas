//
// main.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Main part of the main app runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/window.h"

int main() {
    auto context = runtime::makeContext("");
    context->runWindowed();
    return 0;
}

void RuntimeScene::initialize([[maybe_unused]] Window &window) {}

void RuntimeScene::update([[maybe_unused]] Window &window) {}