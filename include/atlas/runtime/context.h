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

struct ObjectIdentifier {
    std::string type;
    std::string name;
    int id;
};

class RuntimeScene : public Scene {
  public:
    void update(Window &window) override;
    void initialize(Window &window) override;
};

class Context {
  public:
    Context() = default;
    std::string projectRoot;
    std::shared_ptr<RuntimeScene> scene;

    std::shared_ptr<Window> window;
    std::map<ObjectIdentifier, std::shared_ptr<Renderable>> objects;

    void runWindowed();
};

namespace runtime {
std::shared_ptr<Context> makeContext(std::string projectRoot);
};

#endif // RUNTIME_CONTEXT_H