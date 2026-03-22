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

#include <string>
#include <unordered_map>
#include "quickjs.h"

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
void dumpExecution(JSContext *ctx);
bool checkNotException(JSContext *ctx, JSValueConst value, const char *what);
void installGlobals(JSContext *ctx);

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
