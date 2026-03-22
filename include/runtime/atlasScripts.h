#ifndef ATLAS_RUNTIME_SCRIPTS_H
#define ATLAS_RUNTIME_SCRIPTS_H

#include <cstddef>

struct AtlasPackedScriptSource {
    const char *const *parts;
    std::size_t count;
};

struct AtlasRuntimeScriptModule {
    const char *name;
    AtlasPackedScriptSource source;
};

static const char* const ATLAS_PARTS[] = {
    "export class Component {\n    init() {}\n    update(deltaTime) {}\n}",
};
static const AtlasPackedScriptSource ATLAS = {ATLAS_PARTS, 1};

static const char* const ATLAS_LOG_PARTS[] = {
    "\nexport const Debug = {\n    print(message) {\n        globalThis.print(message);\n    },\n    warning(message) {\n        globalThis.print(`[WARNING] ${message}`);\n    },\n    error(message) {\n        globalThis.print(`[ERROR] ${message}`);\n    }\n};",
};
static const AtlasPackedScriptSource ATLAS_LOG = {ATLAS_LOG_PARTS, 1};

static const AtlasRuntimeScriptModule ATLAS_RUNTIME_SCRIPT_MODULES[] = {
    {"atlas", ATLAS},
    {"atlas/log", ATLAS_LOG},
    {"atlas_log", ATLAS_LOG},
};

static constexpr std::size_t ATLAS_RUNTIME_SCRIPT_MODULE_COUNT = sizeof(ATLAS_RUNTIME_SCRIPT_MODULES) / sizeof(ATLAS_RUNTIME_SCRIPT_MODULES[0]);

#endif
