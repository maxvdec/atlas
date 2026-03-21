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

#include "atlas/core/renderable.h"
#include "atlas/scene.h"
#include <atlas/window.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct ObjectIdentifier {
    std::string type;
    std::string name;
    int id;
};

class Context;

class RuntimeScene : public Scene {
  public:
    std::shared_ptr<Context> context;

    void update(Window &window) override;
    void initialize(Window &window) override;
};

class ProjectConfig {
  public:
    std::string renderer;
    bool globalIllumination;
    std::string mainScene;
    bool useUpscaling = false;
    std::vector<std::string> assetDirectories;
};

class Context {
  public:
    Context() = default;
    std::string projectFile;
    std::string projectDir;
    std::shared_ptr<RuntimeScene> scene;

    std::unique_ptr<Window> window;
    std::map<ObjectIdentifier, std::shared_ptr<Renderable>> objects;

    ProjectConfig config;

    void runWindowed();
    void loadProject();
    void loadMainScene(Window &window);
};

namespace runtime {
std::shared_ptr<Context> makeContext(std::string projectFile);
};

#endif // RUNTIME_CONTEXT_H
