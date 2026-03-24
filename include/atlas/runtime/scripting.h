//
// scripting.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Scripting functions and definitions for the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#ifndef RUNTIME_SCRIPTING_H
#define RUNTIME_SCRIPTING_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "quickjs.h"

class Context;
class GameObject;
class Component;
class AudioPlayer;

struct ScriptObjectState {
    GameObject *object = nullptr;
    bool attachedToWindow = false;
};

struct ScriptComponentState {
    Component *component = nullptr;
    int ownerId = 0;
    std::string name;
    JSValue value = JS_UNDEFINED;
};

struct ScriptAudioPlayerState {
    std::shared_ptr<AudioPlayer> component;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptHost {
    Context *context = nullptr;
    std::unordered_map<std::string, std::string> modules;
    std::unordered_map<int, JSValue> objectCache;
    std::unordered_map<int, ScriptObjectState> objectStates;
    std::unordered_map<std::uint64_t, JSValue> instanceCache;
    std::unordered_map<std::uint64_t, ScriptComponentState> componentStates;
    std::unordered_map<Component *, std::uint64_t> componentIds;
    std::unordered_map<int, std::vector<std::uint64_t>> componentOrder;
    std::unordered_map<std::string, std::uint64_t> componentLookup;
    std::unordered_map<std::uint64_t, ScriptAudioPlayerState> audioPlayers;
    JSValue atlasNamespace = JS_UNDEFINED;
    JSValue atlasUnitsNamespace = JS_UNDEFINED;
    JSValue componentPrototype = JS_UNDEFINED;
    JSValue gameObjectPrototype = JS_UNDEFINED;
    JSValue coreObjectPrototype = JS_UNDEFINED;
    JSValue materialPrototype = JS_UNDEFINED;
    JSValue instancePrototype = JS_UNDEFINED;
    JSValue coreVertexPrototype = JS_UNDEFINED;
    JSValue resourcePrototype = JS_UNDEFINED;
    JSValue position3dPrototype = JS_UNDEFINED;
    JSValue position2dPrototype = JS_UNDEFINED;
    JSValue colorPrototype = JS_UNDEFINED;
    JSValue size2dPrototype = JS_UNDEFINED;
    JSValue quaternionPrototype = JS_UNDEFINED;
    std::uint64_t nextComponentId = 1;
    std::uint64_t nextAudioPlayerId = 1;
    std::uint64_t generation = 1;
};

struct ScriptInstance {
    JSContext *ctx = nullptr;
    JSValue instance = JS_UNDEFINED;

    ~ScriptInstance();

    bool callMethod(const char *method, int argc, JSValueConst *argv);
};

namespace runtime::scripting {
void dumpExecution(JSContext *ctx);
bool checkNotException(JSContext *ctx, JSValueConst value, const char *what);
void installGlobals(JSContext *ctx);
void clearSceneBindings(JSContext *ctx, ScriptHost &host);
std::uint64_t registerComponentInstance(JSContext *ctx, ScriptHost &host,
                                        Component *component, int ownerId,
                                        const std::string &name,
                                        JSValueConst value);

char *normalizeModuleName(JSContext *ctx, const char *baseName,
                          const char *name, void *host);
JSModuleDef *loadModule(JSContext *ctx, const char *name, void *opaque);
bool evalModule(JSContext *ctx, const std::string &name, const std::string &src);
JSValue importModuleNamespace(JSContext *ctx, const std::string &name);

ScriptInstance *createScriptInstance(JSContext *ctx,
                                     const std::string &entryModuleName,
                                     const std::string &scriptPath,
                                     const std::string &className);

JSValue jsPrint(JSContext *ctx, JSValueConst this_val, int argc,
                JSValueConst *argv);
} // namespace runtime::scripting

#endif // RUNTIME_SCRIPTING_H
