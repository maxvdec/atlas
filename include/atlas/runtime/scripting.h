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

#include "quickjs.h"
#include "atlas/runtime/context.h"
#include <string>
#include <unordered_map>

struct ScriptHost {
    std::unordered_map<std::string, std::string> modules;
};

struct ScriptInstance {
    JSContext *ctx = nullptr;
    JSValue instance = JS_UNDEFINED;

    ~ScriptInstance();

    bool callMethod(const char *method, int argc, JSValueConst *argv);
};

namespace runtime::scripting {
void installGlobals(JSContext *ctx);

std::string normalizeModuleName(JSContext *ctx, std::string baseName,
                                std::string name, void *host);
JSModuleDef *loadModule(JSContext *ctx, const char *name, void *opaque);
bool evalModule(JSContext *ctx, std::string name, std::string src);
JSValue importModuleNamespace(JSContext *ctx, std::string name);

ScriptInstance *createScriptInstance(JSContext *ctx,
                                     std::string entryModuleName,
                                     std::string scriptPath,
                                     std::string className);

} // namespace runtime::scripting

namespace runtime::scripting {
// Debug functions
JSValue jsPrint(JSContext *ctx, JSValueConst this_val, int argc,
                JSValueConst *argv);
} // namespace runtime::scripting

#endif // RUNTIME_SCRIPTING_H