//
// context.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Context settings for the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/window.h"
#include <memory>

std::shared_ptr<Context> runtime::makeContext(std::string projectRoot) {
    auto context = std::make_shared<Context>();

    context->window = std::make_shared<Window>(Window({.title = "Atlas Runtime",
                                                       .width = 1280,
                                                       .height = 720,
                                                       .renderScale = 1.f,
                                                       .mouseCaptured = false,
                                                       .multisampling = false,
                                                       .ssaoScale = 0.4f}));
    context->projectRoot = std::move(projectRoot);
    context->scene = std::make_shared<RuntimeScene>();

    return context;
}

void Context::runWindowed() {
    window->setScene(scene.get());
    window->run();
}