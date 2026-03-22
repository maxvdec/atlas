//
// context.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Context for the definition of the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#ifndef RUNTIME_CONTEXT_H
#define RUNTIME_CONTEXT_H

#include "atlas/camera.h"
#include "atlas/core/renderable.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include <atlas/window.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <json.hpp>

using json = nlohmann::json;

class Context;

class RuntimeScene : public Scene {
  public:
    std::shared_ptr<Context> context;

    void update(Window &window) override;
    void initialize(Window &window) override;
    void onMouseMove(Window &window, Movement2d movement) override;
    void onMouseScroll(Window &window, Movement2d offset) override;
};

class ProjectConfig {
  public:
    std::string renderer;
    bool globalIllumination;
    std::string mainScene;
    bool useUpscaling = false;
    std::vector<std::string> assetDirectories;
};

#define RUNTIME_LOG(msg)                                                       \
    std::cout << "\033[1;35m[RUINTIME LOG]: \033[0m" << (msg) << std::endl;

class Context {
  public:
    Context() = default;
    std::string projectFile;
    std::string projectDir;
    std::string sceneDir;
    std::shared_ptr<RuntimeScene> scene;

    std::unique_ptr<Camera> camera;
    std::map<std::string, std::unique_ptr<RenderTarget>> renderTargets;
    std::vector<std::unique_ptr<DirectionalLight>> directionalLights;
    std::vector<std::unique_ptr<Light>> pointLights;
    std::vector<std::unique_ptr<Spotlight>> spotlights;
    std::vector<std::unique_ptr<AreaLight>> areaLights;
    std::vector<std::string> cameraActions;
    bool cameraAutomaticMoving = false;

    std::unique_ptr<Window> window;
    std::vector<std::shared_ptr<Renderable>> objects;
    std::unordered_map<std::string, GameObject *> objectReferences;

    ProjectConfig config;

    void runWindowed();
    void loadProject();
    void loadMainScene(Window &window);
    void loadScene(Window &window, const json &sceneData);
};

namespace runtime {
std::shared_ptr<Context> makeContext(std::string projectFile);
};

#endif // RUNTIME_CONTEXT_H
