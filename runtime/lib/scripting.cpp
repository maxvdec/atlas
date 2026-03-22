//
// scripting.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Scripting definitions and code for running scripts
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/scripting.h"
#include "atlas/runtime/context.h"
#include <iostream>
#include <quickjs.h>
#include <string>

const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";
const std::string YELLOW = "\033[33m";

void runtime::scripting::dumpExecution(JSContext *ctx) {
    JSValue exceptionVal = JS_GetException(ctx);
    const char *exceptionStr = JS_ToCString(ctx, exceptionVal);
    if (exceptionStr) {
        std::cout << BOLD << RED << "Script execution failed: " << RESET
                  << YELLOW << exceptionStr << RESET << std::endl;
        JS_FreeCString(ctx, exceptionStr);
    }

    JSValue stack = JS_GetPropertyStr(ctx, exceptionVal, "stack");
    if (!JS_IsUndefined(stack)) {
        const char *stackStr = JS_ToCString(ctx, stack);
        if (stackStr) {
            std::cerr << stackStr << "\n";
            JS_FreeCString(ctx, stackStr);
        }
    }

    JS_FreeValue(ctx, stack);
    JS_FreeValue(ctx, exceptionVal);
}

bool runtime::scripting::checkNotException(JSContext *ctx, JSValue value,
                                           const char *what) {
    if (JS_IsException(value)) {
        std::cout << BOLD << RED << "Error during " << what << ": " << RESET;
        dumpExecution(ctx);
        return false;
    }
    return true;
}

void runtime::scripting::installGlobals(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "print",
                      JS_NewCFunction(ctx, jsPrint, "print", 1));
    JS_FreeValue(ctx, global);
}

std::string runtime::scripting::normalizeModuleName(JSContext *ctx,
                                                    std::string baseName,
                                                    std::string name,
                                                    void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);

    if (host->modules.contains(name)) {
        return js_strdup(ctx, name.c_str());
    }

    if (name[0] == '.') {
        std::string base = baseName.empty() ? "" : baseName;
        auto slash = base.rfind('/');
        std::string dir =
            (slash == std::string::npos) ? "" : base.substr(0, slash + 1);
        std::string resolved = dir + name;

        while (true) {
            auto pos = resolved.find("/./");
            if (pos == std::string::npos)
                break;
            resolved.replace(pos, 3, "/");
        }

        while (true) {
            auto pos = resolved.find("../");
            if (pos == std::string::npos)
                break;
            auto prev = resolved.rfind('/', pos > 1 ? pos - 2 : 0);
            if (prev == std::string::npos)
                break;
            auto next = resolved.find('/', pos + 2);
            resolved.erase(
                prev + 1,
                (next == std::string::npos ? resolved.size() : next + 1) -
                    (prev + 1));
        }

        if (host->modules.contains(resolved)) {
            return js_strdup(ctx, resolved.c_str());
        }
    }

    JS_ThrowReferenceError(ctx, "Could not resolve module '%s' from '%s'",
                           name.c_str(),
                           baseName.empty() ? "<root>" : baseName.c_str());
    return "";
}

JSModuleDef *runtime::scripting::loadModule(JSContext *ctx,
                                            const char *module_name,
                                            void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);

    auto it = host->modules.find(module_name);
    if (it == host->modules.end()) {
        JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
        return nullptr;
    }

    const std::string &source = it->second;

    JSValue func_val = JS_Eval(ctx, source.c_str(), source.size(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func_val)) {
        return nullptr;
    }

    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val));
    JS_FreeValue(ctx, func_val);
    return m;
}

bool runtime::scripting::evalModule(JSContext *ctx, std::string name,
                                    std::string src) {
    JSValue compiled = JS_Eval(ctx, src.c_str(), src.length(), name.c_str(),
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (!checkNotException(ctx, compiled, "compile module")) {
        return false;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (!checkNotException(ctx, result, "execute module")) {
        return false;
    }

    JS_FreeValue(ctx, result);
    return true;
}

JSValue runtime::scripting::importModuleNamespace(JSContext *ctx,
                                                  std::string module_name) {
    std::string src = "import * as ns from '" + module_name +
                      "';\n"
                      "globalThis.__atlas_tmp_ns = ns;\n";

    JSValue compiled = JS_Eval(ctx, src.c_str(), src.size(), "<import_ns>",
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(compiled)) {
        return JS_EXCEPTION;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) {
        return JS_EXCEPTION;
    }
    JS_FreeValue(ctx, result);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ns = JS_GetPropertyStr(ctx, global, "__atlas_tmp_ns");
    JSAtom atom = JS_NewAtom(ctx, "__atlas_tmp_ns");
    JS_DeleteProperty(ctx, global, atom, 0);
    JS_FreeAtom(ctx, atom);
    JS_FreeValue(ctx, global);
    return ns;
}

ScriptInstance::~ScriptInstance() {
    if (ctx && !JS_IsUndefined(instance)) {
        JS_FreeValue(ctx, instance);
    }
}

bool ScriptInstance::callMethod(const char *method_name, int argc = 0,
                                JSValue *argv = nullptr) {
    JSValue fn = JS_GetPropertyStr(ctx, instance, method_name);
    if (JS_IsException(fn)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }

    if (JS_IsUndefined(fn) || !JS_IsFunction(ctx, fn)) {
        JS_FreeValue(ctx, fn);
        std::cerr << "Method not found or not a function: " << method_name
                  << "\n";
        return false;
    }

    JSValue ret = JS_Call(ctx, fn, instance, argc, argv);
    JS_FreeValue(ctx, fn);

    if (JS_IsException(ret)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }

    JS_FreeValue(ctx, ret);
    return true;
}

ScriptInstance *create_script_instance(JSContext *ctx,
                                       const char *entry_module_name,
                                       const char *script_path,
                                       const char *class_name) {
    JSValue ns =
        runtime::scripting::importModuleNamespace(ctx, entry_module_name);
    if (JS_IsException(ns)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    JSValue atlas_scripts = JS_GetPropertyStr(ctx, ns, "default");
    JS_FreeValue(ctx, ns);
    if (JS_IsException(atlas_scripts)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    JSValue script_exports = JS_GetPropertyStr(ctx, atlas_scripts, script_path);
    JS_FreeValue(ctx, atlas_scripts);
    if (JS_IsException(script_exports)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    if (JS_IsUndefined(script_exports)) {
        std::cerr << "Script exports not found for path: " << script_path
                  << "\n";
        JS_FreeValue(ctx, script_exports);
        return nullptr;
    }

    JSValue ctor = JS_GetPropertyStr(ctx, script_exports, class_name);
    JS_FreeValue(ctx, script_exports);
    if (JS_IsException(ctor)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    if (!JS_IsFunction(ctx, ctor)) {
        std::cerr << "Export '" << class_name
                  << "' is not a constructor/function\n";
        JS_FreeValue(ctx, ctor);
        return nullptr;
    }

    JSValue obj = JS_CallConstructor(ctx, ctor, 0, nullptr);
    JS_FreeValue(ctx, ctor);
    if (JS_IsException(obj)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    auto *inst = new ScriptInstance{};
    inst->ctx = ctx;
    inst->instance = obj;
    return inst;
}

JSValue runtime::scripting::jsPrint(JSContext *ctx,
                                    [[maybe_unused]] JSValueConst this_val,
                                    int argc, JSValueConst *argv) {
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            std::cout << str;
            JS_FreeCString(ctx, str);
        } else {
            std::cout << "<non-string value>";
        }
        if (i < argc - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    return JS_UNDEFINED;
}