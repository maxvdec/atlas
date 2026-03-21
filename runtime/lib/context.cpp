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
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <json.hpp>
#include <string>
#include <toml.hpp>
#include <vector>

std::shared_ptr<Context> runtime::makeContext(std::string projectFile) {
    auto context = std::make_shared<Context>();

    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);

    int resWidth = 1280;
    int resHeight = 720;
    bool mouseCaptured = false;
    bool multisampling = false;
    float ssaoScale = 0.4f;

    if (auto *windowTable = configTable["window"].as_table()) {
        if (auto *dimensions = (*windowTable)["dimensions"].as_array()) {
            if (dimensions->size() == 2 && (*dimensions)[0].is_integer() &&
                (*dimensions)[1].is_integer()) {
                resWidth = (*dimensions)[0].as_integer()->get();
                resHeight = (*dimensions)[1].as_integer()->get();
            }
        }
        mouseCaptured = (*windowTable)["mouse_capture"].value_or(false);
        multisampling = (*windowTable)["multisampling"].value_or(false);
        ssaoScale = (*windowTable)["ssaoScale"].value_or(0.4f);
    }

    context->window = std::make_unique<Window>(WindowConfiguration{
        .title = "Atlas Runtime",
        .width = resWidth,
        .height = resHeight,
        .renderScale = 1.f,
        .mouseCaptured = mouseCaptured,
        .multisampling = multisampling,
        .ssaoScale = ssaoScale,
    });
    context->projectFile = std::move(projectFile);
    context->projectDir =
        std::filesystem::path(context->projectFile).parent_path().string();
    context->scene = std::make_shared<RuntimeScene>();
    context->scene->context = context;

    return context;
}

void Context::runWindowed() {
    window->setScene(scene.get());
    window->run();
}

void Context::loadProject() {
    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);

    std::string defaultRenderer = "normal";
    bool globalIllumination = false;
    std::string mainScene = "main.ascene";
    std::vector<std::string> assetDirectories;
    bool useUpscaling = false;

    if (auto *renderer = configTable["renderer"].as_table()) {
        defaultRenderer = (*renderer)["default"].value_or("normal");
        globalIllumination = (*renderer)["global_illumination"].value_or(false);
        useUpscaling = (*renderer)["use_upscaling"].value_or(false);
    }

    if (auto *gameTable = configTable["game"].as_table()) {
        mainScene = (*gameTable)["main_scene"].value_or("main.ascene");

        assetDirectories.clear();
        if (auto *assets = (*gameTable)["assets"].as_array()) {
            for (const auto &assetDir : *assets) {
                if (assetDir.is_string()) {
                    assetDirectories.push_back(assetDir.as_string()->get());
                }
            }
        }
    }

    config.renderer = defaultRenderer;
    config.globalIllumination = globalIllumination;
    config.mainScene = mainScene;
    config.assetDirectories = assetDirectories;
    config.useUpscaling = useUpscaling;
}

void Context::loadMainScene([[maybe_unused]] Window &window) {}
