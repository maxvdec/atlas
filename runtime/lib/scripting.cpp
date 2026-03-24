//
// scripting.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Scripting definitions and code for running scripts
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/scripting.h"
#include "atlas/audio.h"
#include "atlas/object.h"
#include "atlas/runtime/context.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <memory>
#include <quickjs.h>
#include <string>
#include <utility>
#include <vector>

const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";
const std::string YELLOW = "\033[33m";

namespace {

constexpr const char *ATLAS_OBJECT_ID_PROP = "__atlasObjectId";
constexpr const char *ATLAS_COMPONENT_ID_PROP = "__atlasComponentId";
constexpr const char *ATLAS_AUDIO_PLAYER_ID_PROP = "__atlasAudioPlayerId";
constexpr const char *ATLAS_INSTANCE_OWNER_ID_PROP = "__atlasInstanceOwnerId";
constexpr const char *ATLAS_INSTANCE_INDEX_PROP = "__atlasInstanceIndex";
constexpr const char *ATLAS_GENERATION_PROP = "__atlasGeneration";
constexpr const char *ATLAS_IS_CORE_OBJECT_PROP = "__atlasIsCoreObject";
constexpr const char *ATLAS_NATIVE_COMPONENT_KIND_PROP =
    "__atlasNativeComponentKind";

ScriptHost *getHost(JSContext *ctx) {
    return static_cast<ScriptHost *>(JS_GetContextOpaque(ctx));
}

std::string normalizeToken(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '.') {
            continue;
        }
        normalized.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

std::string makeComponentLookupKey(int ownerId, const std::string &name) {
    return std::to_string(ownerId) + ":" + normalizeToken(name);
}

ResourceType toNativeResourceType(int type) {
    switch (type) {
    case 1:
        return ResourceType::Image;
    case 2:
        return ResourceType::SpecularMap;
    case 3:
        return ResourceType::Audio;
    case 4:
        return ResourceType::Font;
    case 5:
        return ResourceType::Model;
    default:
        return ResourceType::File;
    }
}

int toScriptResourceType(ResourceType type) {
    switch (type) {
    case ResourceType::Image:
        return 1;
    case ResourceType::SpecularMap:
        return 2;
    case ResourceType::Audio:
        return 3;
    case ResourceType::Font:
        return 4;
    case ResourceType::Model:
        return 5;
    case ResourceType::File:
    default:
        return 0;
    }
}

std::uint64_t makeInstanceKey(int ownerId, std::uint32_t index) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(ownerId))
            << 32) |
           static_cast<std::uint64_t>(index);
}

bool setProperty(JSContext *ctx, JSValue obj, const char *name, JSValue value) {
    if (JS_SetPropertyStr(ctx, obj, name, value) < 0) {
        JS_FreeValue(ctx, value);
        return false;
    }
    return true;
}

bool getInt64(JSContext *ctx, JSValueConst value, std::int64_t &out) {
    int64_t temp = 0;
    if (JS_ToInt64(ctx, &temp, value) < 0) {
        return false;
    }
    out = static_cast<std::int64_t>(temp);
    return true;
}

bool getUint32(JSContext *ctx, JSValueConst value, std::uint32_t &out) {
    uint32_t temp = 0;
    if (JS_ToUint32(ctx, &temp, value) < 0) {
        return false;
    }
    out = temp;
    return true;
}

bool getDouble(JSContext *ctx, JSValueConst value, double &out) {
    double temp = 0.0;
    if (JS_ToFloat64(ctx, &temp, value) < 0) {
        return false;
    }
    out = temp;
    return true;
}

bool readNumberProperty(JSContext *ctx, JSValueConst obj, const char *name,
                        double &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    bool ok = !JS_IsUndefined(value) && getDouble(ctx, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

bool readBoolProperty(JSContext *ctx, JSValueConst obj, const char *name,
                      bool &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    if (JS_IsUndefined(value)) {
        JS_FreeValue(ctx, value);
        return false;
    }
    out = JS_ToBool(ctx, value) == 1;
    JS_FreeValue(ctx, value);
    return true;
}

bool readIntProperty(JSContext *ctx, JSValueConst obj, const char *name,
                     std::int64_t &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    bool ok = !JS_IsUndefined(value) && getInt64(ctx, value, out);
    JS_FreeValue(ctx, value);
    return ok;
}

bool readStringProperty(JSContext *ctx, JSValueConst obj, const char *name,
                        std::string &out) {
    JSValue value = JS_GetPropertyStr(ctx, obj, name);
    if (JS_IsException(value)) {
        return false;
    }
    if (JS_IsUndefined(value)) {
        JS_FreeValue(ctx, value);
        return false;
    }
    const char *str = JS_ToCString(ctx, value);
    if (str == nullptr) {
        JS_FreeValue(ctx, value);
        return false;
    }
    out = str;
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, value);
    return true;
}

bool ensureCurrentGeneration(JSContext *ctx, ScriptHost &host,
                             JSValueConst obj) {
    std::int64_t generation = 0;
    if (!readIntProperty(ctx, obj, ATLAS_GENERATION_PROP, generation)) {
        return true;
    }
    if (static_cast<std::uint64_t>(generation) == host.generation) {
        return true;
    }
    JS_ThrowReferenceError(ctx, "Stale Atlas script handle");
    return false;
}

GameObject *findObjectById(ScriptHost &host, int id) {
    auto atlasIt = atlas::gameObjects.find(id);
    if (atlasIt != atlas::gameObjects.end()) {
        return atlasIt->second;
    }

    if (host.context == nullptr) {
        return nullptr;
    }

    auto stateIt = host.objectStates.find(id);
    if (stateIt != host.objectStates.end()) {
        return stateIt->second.object;
    }

    return nullptr;
}

GameObject *findObjectByName(ScriptHost &host, const std::string &name) {
    if (host.context == nullptr) {
        return nullptr;
    }

    auto it = host.context->objectReferences.find(name);
    if (it != host.context->objectReferences.end()) {
        return it->second;
    }

    const std::string normalized = normalizeToken(name);
    it = host.context->objectReferences.find(normalized);
    if (it != host.context->objectReferences.end()) {
        return it->second;
    }

    return nullptr;
}

void assignObjectName(Context &context, GameObject &object,
                      const std::string &name) {
    const int objectId = object.getId();
    auto oldIt = context.objectNames.find(objectId);
    if (oldIt != context.objectNames.end()) {
        const std::string oldName = oldIt->second;
        auto eraseIfMatches = [&](const std::string &key) {
            auto refIt = context.objectReferences.find(key);
            if (refIt != context.objectReferences.end() &&
                refIt->second == &object) {
                context.objectReferences.erase(refIt);
            }
        };
        if (!oldName.empty() && oldName != name) {
            eraseIfMatches(oldName);
            eraseIfMatches(normalizeToken(oldName));
        }
    }

    if (name.empty()) {
        context.objectNames.erase(objectId);
        return;
    }

    auto ensureAvailable = [&](const std::string &key) {
        if (key.empty()) {
            return true;
        }
        auto refIt = context.objectReferences.find(key);
        return refIt == context.objectReferences.end() ||
               refIt->second == &object;
    };

    const std::string normalized = normalizeToken(name);
    if (!ensureAvailable(name) || !ensureAvailable(normalized)) {
        throw std::runtime_error("Duplicate object reference: " + name);
    }

    context.objectReferences[name] = &object;
    context.objectReferences[normalized] = &object;
    context.objectNames[objectId] = name;
}

void cachePrototype(JSContext *ctx, JSValueConst ns, const char *exportName,
                    JSValue &target) {
    if (!JS_IsUndefined(target)) {
        return;
    }

    JSValue ctor = JS_GetPropertyStr(ctx, ns, exportName);
    if (JS_IsException(ctor) || JS_IsUndefined(ctor)) {
        JS_FreeValue(ctx, ctor);
        return;
    }

    target = JS_GetPropertyStr(ctx, ctor, "prototype");
    JS_FreeValue(ctx, ctor);
}

bool ensureBuiltins(JSContext *ctx, ScriptHost &host) {
    if (JS_IsUndefined(host.atlasNamespace)) {
        host.atlasNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas");
        if (JS_IsException(host.atlasNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasUnitsNamespace)) {
        host.atlasUnitsNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/units");
        if (JS_IsException(host.atlasUnitsNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    cachePrototype(ctx, host.atlasNamespace, "Component",
                   host.componentPrototype);
    cachePrototype(ctx, host.atlasNamespace, "GameObject",
                   host.gameObjectPrototype);
    cachePrototype(ctx, host.atlasNamespace, "CoreObject",
                   host.coreObjectPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Material",
                   host.materialPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Instance",
                   host.instancePrototype);
    cachePrototype(ctx, host.atlasNamespace, "CoreVertex",
                   host.coreVertexPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Resource",
                   host.resourcePrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Position3d",
                   host.position3dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Position2d",
                   host.position2dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Color", host.colorPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Size2d",
                   host.size2dPrototype);
    cachePrototype(ctx, host.atlasUnitsNamespace, "Quaternion",
                   host.quaternionPrototype);
    return true;
}

JSValue newObjectFromPrototype(JSContext *ctx, JSValueConst prototype) {
    if (JS_IsUndefined(prototype)) {
        return JS_NewObject(ctx);
    }
    return JS_NewObjectProto(ctx, prototype);
}

JSValue makePosition3d(JSContext *ctx, ScriptHost &host,
                       const Position3d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position3dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    setProperty(ctx, result, "z", JS_NewFloat64(ctx, value.z));
    return result;
}

JSValue makePosition2d(JSContext *ctx, ScriptHost &host,
                       const Position2d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position2dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    return result;
}

JSValue makeColor(JSContext *ctx, ScriptHost &host, const Color &value) {
    JSValue result = newObjectFromPrototype(ctx, host.colorPrototype);
    setProperty(ctx, result, "r", JS_NewFloat64(ctx, value.r));
    setProperty(ctx, result, "g", JS_NewFloat64(ctx, value.g));
    setProperty(ctx, result, "b", JS_NewFloat64(ctx, value.b));
    setProperty(ctx, result, "a", JS_NewFloat64(ctx, value.a));
    return result;
}

JSValue makeResource(JSContext *ctx, ScriptHost &host, const Resource &resource) {
    JSValue result = newObjectFromPrototype(ctx, host.resourcePrototype);
    setProperty(ctx, result, "type",
                JS_NewInt32(ctx, toScriptResourceType(resource.type)));
    setProperty(ctx, result, "path",
                JS_NewString(ctx, resource.path.string().c_str()));
    setProperty(ctx, result, "name", JS_NewString(ctx, resource.name.c_str()));
    return result;
}

bool parsePosition3d(JSContext *ctx, JSValueConst value, Position3d &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z)) {
        return false;
    }
    out = Position3d(x, y, z);
    return true;
}

JSValue makeRotation3d(JSContext *ctx, ScriptHost &host,
                       const Rotation3d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.position3dPrototype);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.pitch));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.yaw));
    setProperty(ctx, result, "z", JS_NewFloat64(ctx, value.roll));
    return result;
}

bool parseRotation3d(JSContext *ctx, JSValueConst value, Rotation3d &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z)) {
        return false;
    }
    out = Rotation3d(x, y, z);
    return true;
}

bool parseSize2d(JSContext *ctx, JSValueConst value, Size2d &out) {
    double width = 0.0;
    double height = 0.0;
    if (!readNumberProperty(ctx, value, "width", width)) {
        if (!readNumberProperty(ctx, value, "x", width)) {
            return false;
        }
    }
    if (!readNumberProperty(ctx, value, "height", height)) {
        if (!readNumberProperty(ctx, value, "y", height)) {
            return false;
        }
    }
    out = Size2d(width, height);
    return true;
}

bool parseColor(JSContext *ctx, JSValueConst value, Color &out) {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 1.0;
    if (!readNumberProperty(ctx, value, "r", r) ||
        !readNumberProperty(ctx, value, "g", g) ||
        !readNumberProperty(ctx, value, "b", b)) {
        return false;
    }
    readNumberProperty(ctx, value, "a", a);
    out = Color(r, g, b, a);
    return true;
}

bool parseResource(JSContext *ctx, JSValueConst value, Resource &out) {
    std::string path;
    std::string name;
    std::int64_t type = 0;

    if (!readStringProperty(ctx, value, "path", path) ||
        !readStringProperty(ctx, value, "name", name)) {
        return false;
    }

    readIntProperty(ctx, value, "type", type);
    out.path = path;
    out.name = name;
    out.type = toNativeResourceType(static_cast<int>(type));
    return true;
}

bool parseQuaternion(JSContext *ctx, JSValueConst value, glm::quat &out) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y) ||
        !readNumberProperty(ctx, value, "z", z) ||
        !readNumberProperty(ctx, value, "w", w)) {
        return false;
    }
    out = glm::normalize(
        glm::quat(static_cast<float>(w), static_cast<float>(x),
                  static_cast<float>(y), static_cast<float>(z)));
    return true;
}

bool parseMaterial(JSContext *ctx, JSValueConst value, Material &out) {
    Color albedo = out.albedo;
    JSValue albedoValue = JS_GetPropertyStr(ctx, value, "albedo");
    if (!JS_IsException(albedoValue) && !JS_IsUndefined(albedoValue)) {
        parseColor(ctx, albedoValue, albedo);
    }
    JS_FreeValue(ctx, albedoValue);
    out.albedo = albedo;

    double metallic = out.metallic;
    double roughness = out.roughness;
    double ao = out.ao;
    double reflectivity = out.reflectivity;
    double emissiveIntensity = out.emissiveIntensity;
    double normalMapStrength = out.normalMapStrength;
    double transmittance = out.transmittance;
    double ior = out.ior;
    bool useNormalMap = out.useNormalMap;
    Color emissiveColor = out.emissiveColor;

    readNumberProperty(ctx, value, "metallic", metallic);
    readNumberProperty(ctx, value, "roughness", roughness);
    readNumberProperty(ctx, value, "ao", ao);
    readNumberProperty(ctx, value, "reflectivity", reflectivity);
    readNumberProperty(ctx, value, "emissiveIntensity", emissiveIntensity);
    readNumberProperty(ctx, value, "normalMapStrength", normalMapStrength);
    readNumberProperty(ctx, value, "transmittance", transmittance);
    readNumberProperty(ctx, value, "ior", ior);
    readBoolProperty(ctx, value, "useNormalMap", useNormalMap);

    JSValue emissiveValue = JS_GetPropertyStr(ctx, value, "emissiveColor");
    if (!JS_IsException(emissiveValue) && !JS_IsUndefined(emissiveValue)) {
        parseColor(ctx, emissiveValue, emissiveColor);
    }
    JS_FreeValue(ctx, emissiveValue);

    out.metallic = static_cast<float>(metallic);
    out.roughness = static_cast<float>(roughness);
    out.ao = static_cast<float>(ao);
    out.reflectivity = static_cast<float>(reflectivity);
    out.emissiveColor = emissiveColor;
    out.emissiveIntensity = static_cast<float>(emissiveIntensity);
    out.normalMapStrength = static_cast<float>(normalMapStrength);
    out.useNormalMap = useNormalMap;
    out.transmittance = static_cast<float>(transmittance);
    out.ior = static_cast<float>(ior);
    return true;
}

ScriptAudioPlayerState *findAudioPlayerState(ScriptHost &host,
                                             std::uint64_t audioPlayerId) {
    auto it = host.audioPlayers.find(audioPlayerId);
    if (it == host.audioPlayers.end()) {
        return nullptr;
    }
    return &it->second;
}

bool parseCoreVertex(JSContext *ctx, JSValueConst value, CoreVertex &out) {
    JSValue prop = JS_GetPropertyStr(ctx, value, "position");
    if (JS_IsException(prop) || !parsePosition3d(ctx, prop, out.position)) {
        JS_FreeValue(ctx, prop);
        return false;
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "color");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parseColor(ctx, prop, out.color);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "textureCoord");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        double x = 0.0;
        double y = 0.0;
        if (readNumberProperty(ctx, prop, "x", x) &&
            readNumberProperty(ctx, prop, "y", y)) {
            out.textureCoordinate = {static_cast<float>(x),
                                     static_cast<float>(y)};
        }
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "normal");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.normal);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "tangent");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.tangent);
    }
    JS_FreeValue(ctx, prop);

    prop = JS_GetPropertyStr(ctx, value, "bitangent");
    if (!JS_IsException(prop) && !JS_IsUndefined(prop)) {
        parsePosition3d(ctx, prop, out.bitangent);
    }
    JS_FreeValue(ctx, prop);

    return true;
}

JSValue buildComponentsArray(JSContext *ctx, ScriptHost &host, int ownerId) {
    JSValue result = JS_NewArray(ctx);
    std::uint32_t writeIndex = 0;

    auto orderIt = host.componentOrder.find(ownerId);
    if (orderIt == host.componentOrder.end()) {
        return result;
    }

    for (std::uint64_t componentId : orderIt->second) {
        auto stateIt = host.componentStates.find(componentId);
        if (stateIt == host.componentStates.end()) {
            continue;
        }
        if (stateIt->second.ownerId != ownerId) {
            continue;
        }
        JS_SetPropertyUint32(ctx, result, writeIndex++,
                             JS_DupValue(ctx, stateIt->second.value));
    }

    return result;
}

void trackObjectState(ScriptHost &host, GameObject &object, bool attached) {
    auto &state = host.objectStates[object.getId()];
    state.object = &object;
    state.attachedToWindow = state.attachedToWindow || attached;
}

void attachObjectIfReady(ScriptHost &host, GameObject &object) {
    if (host.context == nullptr || host.context->window == nullptr) {
        return;
    }

    auto &state = host.objectStates[object.getId()];
    state.object = &object;
    if (state.attachedToWindow) {
        return;
    }

    if (auto *core = dynamic_cast<CoreObject *>(&object); core != nullptr &&
        core->vertices.empty()) {
        return;
    }

    object.initialize();
    host.context->window->addObject(&object);
    state.attachedToWindow = true;
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host, GameObject &object);

JSValue syncInstanceWrapper(JSContext *ctx, ScriptHost &host, CoreObject &object,
                            std::uint32_t index) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    if (index >= object.instances.size()) {
        return JS_NULL;
    }

    const std::uint64_t key = makeInstanceKey(object.getId(), index);
    JSValue wrapper = JS_UNDEFINED;
    auto cacheIt = host.instanceCache.find(key);
    if (cacheIt != host.instanceCache.end()) {
        wrapper = JS_DupValue(ctx, cacheIt->second);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.instancePrototype);
        host.instanceCache[key] = JS_DupValue(ctx, wrapper);
    }

    const Instance &instance = object.instances[index];
    setProperty(ctx, wrapper, ATLAS_INSTANCE_OWNER_ID_PROP,
                JS_NewInt32(ctx, object.getId()));
    setProperty(ctx, wrapper, ATLAS_INSTANCE_INDEX_PROP,
                JS_NewInt32(ctx, static_cast<int>(index)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position", makePosition3d(ctx, host, instance.position));
    setProperty(ctx, wrapper, "rotation", makeRotation3d(ctx, host, instance.rotation));
    setProperty(ctx, wrapper, "scale", makePosition3d(ctx, host, instance.scale));
    return wrapper;
}

JSValue makeMaterial(JSContext *ctx, ScriptHost &host, const Material &material) {
    JSValue result = newObjectFromPrototype(ctx, host.materialPrototype);
    setProperty(ctx, result, "albedo", makeColor(ctx, host, material.albedo));
    setProperty(ctx, result, "metallic", JS_NewFloat64(ctx, material.metallic));
    setProperty(ctx, result, "roughness",
                JS_NewFloat64(ctx, material.roughness));
    setProperty(ctx, result, "ao", JS_NewFloat64(ctx, material.ao));
    setProperty(ctx, result, "reflectivity",
                JS_NewFloat64(ctx, material.reflectivity));
    setProperty(ctx, result, "emissiveColor",
                makeColor(ctx, host, material.emissiveColor));
    setProperty(ctx, result, "emissiveIntensity",
                JS_NewFloat64(ctx, material.emissiveIntensity));
    setProperty(ctx, result, "normalMapStrength",
                JS_NewFloat64(ctx, material.normalMapStrength));
    setProperty(ctx, result, "useNormalMap",
                JS_NewBool(ctx, material.useNormalMap));
    setProperty(ctx, result, "transmittance",
                JS_NewFloat64(ctx, material.transmittance));
    setProperty(ctx, result, "ior", JS_NewFloat64(ctx, material.ior));
    return result;
}

JSValue makeCoreVertex(JSContext *ctx, ScriptHost &host, const CoreVertex &vertex) {
    JSValue result = newObjectFromPrototype(ctx, host.coreVertexPrototype);
    setProperty(ctx, result, "position", makePosition3d(ctx, host, vertex.position));
    setProperty(ctx, result, "color", makeColor(ctx, host, vertex.color));
    setProperty(ctx, result, "textureCoord",
                makePosition2d(ctx, host, Position2d(vertex.textureCoordinate[0],
                                                     vertex.textureCoordinate[1])));
    setProperty(ctx, result, "normal", makePosition3d(ctx, host, vertex.normal));
    setProperty(ctx, result, "tangent", makePosition3d(ctx, host, vertex.tangent));
    setProperty(ctx, result, "bitangent",
                makePosition3d(ctx, host, vertex.bitangent));
    return result;
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host, GameObject &object) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    const int objectId = object.getId();
    JSValue wrapper = JS_UNDEFINED;
    auto cacheIt = host.objectCache.find(objectId);
    if (cacheIt != host.objectCache.end()) {
        wrapper = JS_DupValue(ctx, cacheIt->second);
    } else {
        JSValueConst prototype =
            dynamic_cast<CoreObject *>(&object) != nullptr
                ? host.coreObjectPrototype
                : host.gameObjectPrototype;
        wrapper = newObjectFromPrototype(ctx, prototype);
        host.objectCache[objectId] = JS_DupValue(ctx, wrapper);
    }

    auto stateIt = host.objectStates.find(objectId);
    const bool attached = stateIt != host.objectStates.end()
                              ? stateIt->second.attachedToWindow
                              : true;
    trackObjectState(host, object, attached);

    setProperty(ctx, wrapper, ATLAS_OBJECT_ID_PROP, JS_NewInt32(ctx, objectId));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, ATLAS_IS_CORE_OBJECT_PROP,
                JS_NewBool(ctx, dynamic_cast<CoreObject *>(&object) != nullptr));
    setProperty(ctx, wrapper, "id", JS_NewInt32(ctx, objectId));
    setProperty(ctx, wrapper, "components", buildComponentsArray(ctx, host, objectId));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, object.getPosition()));
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, object.getRotation()));
    setProperty(ctx, wrapper, "scale",
                makePosition3d(ctx, host, object.getScale()));

    std::string name;
    if (host.context != nullptr) {
        auto nameIt = host.context->objectNames.find(objectId);
        if (nameIt != host.context->objectNames.end()) {
            name = nameIt->second;
        }
    }
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, name.c_str()));

    if (auto *core = dynamic_cast<CoreObject *>(&object); core != nullptr) {
        JSValue vertices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->vertices.size(); ++i) {
            JS_SetPropertyUint32(ctx, vertices, i,
                                 makeCoreVertex(ctx, host, core->vertices[i]));
        }

        JSValue indices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->indices.size(); ++i) {
            JS_SetPropertyUint32(ctx, indices, i,
                                 JS_NewInt32(ctx, static_cast<int>(core->indices[i])));
        }

        JSValue instances = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->instances.size(); ++i) {
            JS_SetPropertyUint32(ctx, instances, i,
                                 syncInstanceWrapper(ctx, host, *core, i));
        }

        setProperty(ctx, wrapper, "vertices", vertices);
        setProperty(ctx, wrapper, "indices", indices);
        setProperty(ctx, wrapper, "textures", JS_NewArray(ctx));
        setProperty(ctx, wrapper, "material", makeMaterial(ctx, host, core->material));
        setProperty(ctx, wrapper, "instances", instances);
        setProperty(ctx, wrapper, "castsShadows",
                    JS_NewBool(ctx, core->castsShadows));
    }

    return wrapper;
}

bool applyBaseObject(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     GameObject &object) {
    Position3d position = object.getPosition();
    Rotation3d rotation = object.getRotation();
    Scale3d scale = object.getScale();

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "scale");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, scale);
    }
    JS_FreeValue(ctx, value);

    object.setPosition(position);
    object.setRotation(rotation);
    object.setScale(scale);

    if (host.context != nullptr) {
        std::string name;
        if (readStringProperty(ctx, wrapper, "name", name)) {
            assignObjectName(*host.context, object, name);
        }
    }

    return true;
}

bool applyCoreObject(JSContext *ctx, ScriptHost &host, JSValueConst wrapper,
                     CoreObject &object) {
    applyBaseObject(ctx, host, wrapper, object);

    bool geometryUpdated = false;
    JSValue vertices = JS_GetPropertyStr(ctx, wrapper, "vertices");
    if (!JS_IsException(vertices) && JS_IsArray(vertices)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, vertices, "length");
        getUint32(ctx, lengthValue, length);
        JS_FreeValue(ctx, lengthValue);

        std::vector<CoreVertex> parsedVertices;
        parsedVertices.reserve(length);
        for (std::uint32_t i = 0; i < length; ++i) {
            JSValue vertexValue = JS_GetPropertyUint32(ctx, vertices, i);
            if (JS_IsException(vertexValue)) {
                JS_FreeValue(ctx, vertices);
                return false;
            }
            CoreVertex vertex;
            if (parseCoreVertex(ctx, vertexValue, vertex)) {
                parsedVertices.push_back(vertex);
            }
            JS_FreeValue(ctx, vertexValue);
        }
        if (!parsedVertices.empty()) {
            object.attachVertices(parsedVertices);
            geometryUpdated = true;
        }
    }
    JS_FreeValue(ctx, vertices);

    JSValue indices = JS_GetPropertyStr(ctx, wrapper, "indices");
    if (!JS_IsException(indices) && JS_IsArray(indices)) {
        std::uint32_t length = 0;
        JSValue lengthValue = JS_GetPropertyStr(ctx, indices, "length");
        getUint32(ctx, lengthValue, length);
        JS_FreeValue(ctx, lengthValue);

        std::vector<Index> parsedIndices;
        parsedIndices.reserve(length);
        for (std::uint32_t i = 0; i < length; ++i) {
            JSValue indexValue = JS_GetPropertyUint32(ctx, indices, i);
            if (JS_IsException(indexValue)) {
                JS_FreeValue(ctx, indices);
                return false;
            }
            std::int64_t parsedIndex = 0;
            if (getInt64(ctx, indexValue, parsedIndex)) {
                parsedIndices.push_back(static_cast<Index>(parsedIndex));
            }
            JS_FreeValue(ctx, indexValue);
        }
        object.attachIndices(parsedIndices);
        geometryUpdated = true;
    }
    JS_FreeValue(ctx, indices);

    JSValue materialValue = JS_GetPropertyStr(ctx, wrapper, "material");
    if (!JS_IsException(materialValue) && !JS_IsUndefined(materialValue)) {
        Material material = object.material;
        parseMaterial(ctx, materialValue, material);
        object.material = material;
    }
    JS_FreeValue(ctx, materialValue);

    bool castsShadows = object.castsShadows;
    if (readBoolProperty(ctx, wrapper, "castsShadows", castsShadows)) {
        object.castsShadows = castsShadows;
    }

    if (geometryUpdated) {
        attachObjectIfReady(host, object);
    }

    return true;
}

GameObject *resolveObjectArg(JSContext *ctx, ScriptHost &host,
                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t objectId = 0;
    if (!readIntProperty(ctx, value, ATLAS_OBJECT_ID_PROP, objectId) &&
        !readIntProperty(ctx, value, "id", objectId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas object handle");
        return nullptr;
    }

    GameObject *object = findObjectById(host, static_cast<int>(objectId));
    if (object == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                               static_cast<int>(objectId));
    }
    return object;
}

CoreObject *resolveCoreObjectArg(JSContext *ctx, ScriptHost &host,
                                 JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *core = dynamic_cast<CoreObject *>(object);
    if (core == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a CoreObject");
        return nullptr;
    }
    return core;
}

Instance *resolveInstanceArg(JSContext *ctx, ScriptHost &host,
                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t ownerId = 0;
    std::int64_t index = 0;
    if (!readIntProperty(ctx, value, ATLAS_INSTANCE_OWNER_ID_PROP, ownerId) ||
        !readIntProperty(ctx, value, ATLAS_INSTANCE_INDEX_PROP, index)) {
        JS_ThrowTypeError(ctx, "Expected Atlas instance handle");
        return nullptr;
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(host, static_cast<int>(ownerId)));
    if (object == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                               static_cast<int>(ownerId));
        return nullptr;
    }

    if (index < 0 ||
        static_cast<std::size_t>(index) >= object->instances.size()) {
        JS_ThrowRangeError(ctx, "Invalid Atlas instance index");
        return nullptr;
    }

    return &object->instances[static_cast<std::size_t>(index)];
}

JSValue jsGetObjectById(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_NULL;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object == nullptr) {
        return JS_NULL;
    }
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetObjectByName(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_NULL;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected object name");
    }

    GameObject *object = findObjectByName(*host, name);
    JS_FreeCString(ctx, name);
    if (object == nullptr) {
        return JS_NULL;
    }
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetComponentByName(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_NULL;
    }

    std::int64_t ownerId = 0;
    if (!getInt64(ctx, argv[0], ownerId)) {
        return JS_ThrowTypeError(ctx, "Expected owner id");
    }

    const char *name = JS_ToCString(ctx, argv[1]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected component name");
    }

    auto lookupIt = host->componentLookup.find(
        makeComponentLookupKey(static_cast<int>(ownerId), name));
    JS_FreeCString(ctx, name);
    if (lookupIt == host->componentLookup.end()) {
        return JS_NULL;
    }

    auto componentIt = host->componentStates.find(lookupIt->second);
    if (componentIt == host->componentStates.end()) {
        return JS_NULL;
    }

    return JS_DupValue(ctx, componentIt->second.value);
}

JSValue jsLoadResource(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected path and resource type");
    }

    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    const char *path = JS_ToCString(ctx, argv[0]);
    if (path == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected resource path");
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        JS_FreeCString(ctx, path);
        return JS_ThrowTypeError(ctx, "Expected resource type");
    }

    std::string name;
    if (argc > 2 && !JS_IsUndefined(argv[2])) {
        const char *providedName = JS_ToCString(ctx, argv[2]);
        if (providedName != nullptr) {
            name = providedName;
            JS_FreeCString(ctx, providedName);
        }
    }

    std::filesystem::path resourcePath(path);
    if (name.empty()) {
        name = resourcePath.stem().string();
        if (name.empty()) {
            name = resourcePath.filename().string();
        }
    }

    Resource resource = Workspace::get().createResource(
        resourcePath, name, toNativeResourceType(static_cast<int>(type)));
    JS_FreeCString(ctx, path);
    return makeResource(ctx, *host, resource);
}

JSValue jsGetResourceByName(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource name");
    }

    if (!ensureBuiltins(ctx, *host)) {
        return JS_EXCEPTION;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected resource name");
    }

    Resource resource = Workspace::get().getResource(name);
    JS_FreeCString(ctx, name);

    if (resource.name.empty()) {
        return JS_NULL;
    }

    if (argc > 1 && !JS_IsUndefined(argv[1])) {
        std::int64_t expectedType = 0;
        if (getInt64(ctx, argv[1], expectedType) &&
            resource.type != toNativeResourceType(static_cast<int>(expectedType))) {
            return JS_NULL;
        }
    }

    return makeResource(ctx, *host, resource);
}

JSValue jsCreateAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player wrapper");
    }

    std::uint64_t audioPlayerId = host->nextAudioPlayerId++;
    auto component = std::make_shared<AudioPlayer>();

    host->audioPlayers[audioPlayerId] = {.component = component,
                                         .value = JS_DupValue(ctx, argv[0]),
                                         .attached = false};

    JSValue wrapper = JS_DupValue(ctx, argv[0]);
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(audioPlayerId)));
    setProperty(ctx, wrapper, ATLAS_AUDIO_PLAYER_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(audioPlayerId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host->generation)));
    setProperty(ctx, wrapper, ATLAS_NATIVE_COMPONENT_KIND_PROP,
                JS_NewString(ctx, "audio-player"));
    JS_FreeValue(ctx, wrapper);

    return JS_UNDEFINED;
}

ScriptAudioPlayerState *resolveAudioPlayer(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    std::int64_t audioPlayerId = 0;
    if (!getInt64(ctx, value, audioPlayerId)) {
        JS_ThrowTypeError(ctx, "Expected audio player id");
        return nullptr;
    }

    ScriptAudioPlayerState *state =
        findAudioPlayerState(host, static_cast<std::uint64_t>(audioPlayerId));
    if (state == nullptr || !state->component) {
        JS_ThrowReferenceError(ctx, "Unknown audio player id");
        return nullptr;
    }

    return state;
}

JSValue jsInitAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->init();
    return JS_UNDEFINED;
}

JSValue jsPlayAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->play();
    return JS_UNDEFINED;
}

JSValue jsPauseAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->pause();
    return JS_UNDEFINED;
}

JSValue jsStopAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected audio player id");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->stop();
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerVolume(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and volume");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double volume = 1.0;
    if (!getDouble(ctx, argv[1], volume)) {
        return JS_ThrowTypeError(ctx, "Expected volume");
    }

    state->component->setVolume(static_cast<float>(volume));
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerLoop(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and loop flag");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->component->setLoop(JS_ToBool(ctx, argv[1]) == 1);
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerSource(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and resource");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    Resource resource;
    if (!parseResource(ctx, argv[1], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource");
    }

    state->component->setSource(resource);
    return JS_UNDEFINED;
}

JSValue jsUpdateAudioPlayer(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and delta time");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    double deltaTime = 0.0;
    if (!getDouble(ctx, argv[1], deltaTime)) {
        return JS_ThrowTypeError(ctx, "Expected delta time");
    }

    state->component->update(static_cast<float>(deltaTime));
    return JS_UNDEFINED;
}

JSValue jsSetAudioPlayerPosition(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected audio player id and position");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    state->component->setPosition(position);
    return JS_UNDEFINED;
}

JSValue jsUseSpatialAudio(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected audio player id and enabled flag");
    }

    auto *state = resolveAudioPlayer(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    if (JS_ToBool(ctx, argv[1]) == 1) {
        state->component->useSpatialization();
    } else {
        state->component->disableSpatialization();
    }

    return JS_UNDEFINED;
}

class HostedScriptComponent final : public Component {
  public:
    HostedScriptComponent(JSContext *context, ScriptHost *scriptHost,
                          JSValueConst value, std::string componentName)
        : ctx(context), host(scriptHost), name(std::move(componentName)),
          instance(JS_DupValue(context, value)) {}

    ~HostedScriptComponent() override {
        if (ctx != nullptr && !JS_IsUndefined(instance)) {
            JS_FreeValue(ctx, instance);
        }
    }

    void atAttach() override {
        if (host == nullptr || object == nullptr) {
            return;
        }
        runtime::scripting::registerComponentInstance(
            ctx, *host, this, object->getId(), name, instance);
    }

    void init() override {
        if (!initialized) {
            initialized = true;
            call("init", 0, nullptr);
        }
    }

    void update(float deltaTime) override {
        JSValue delta = JS_NewFloat64(ctx, deltaTime);
        JSValueConst args[] = {delta};
        call("update", 1, args);
        JS_FreeValue(ctx, delta);
    }

  private:
    bool call(const char *method, int argc, JSValueConst *argv) {
        JSValue fn = JS_GetPropertyStr(ctx, instance, method);
        if (JS_IsException(fn)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
        if (JS_IsUndefined(fn) || !JS_IsFunction(ctx, fn)) {
            JS_FreeValue(ctx, fn);
            return false;
        }
        JSValue result = JS_Call(ctx, fn, instance, argc, argv);
        JS_FreeValue(ctx, fn);
        if (JS_IsException(result)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
        JS_FreeValue(ctx, result);
        return true;
    }

    JSContext *ctx = nullptr;
    ScriptHost *host = nullptr;
    std::string name;
    JSValue instance = JS_UNDEFINED;
    bool initialized = false;
};

JSValue jsAddComponent(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object id and component");
    }

    std::int64_t ownerId = 0;
    if (!getInt64(ctx, argv[0], ownerId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(ownerId));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown Atlas object id: %d",
                                      static_cast<int>(ownerId));
    }

    std::string nativeKind;
    if (readStringProperty(ctx, argv[1], ATLAS_NATIVE_COMPONENT_KIND_PROP,
                           nativeKind) &&
        nativeKind == "audio-player") {
        std::int64_t audioPlayerId = 0;
        if (!readIntProperty(ctx, argv[1], ATLAS_AUDIO_PLAYER_ID_PROP,
                             audioPlayerId)) {
            return JS_ThrowReferenceError(ctx, "AudioPlayer wrapper is invalid");
        }

        auto *audioState = findAudioPlayerState(
            *host, static_cast<std::uint64_t>(audioPlayerId));
        if (audioState == nullptr || !audioState->component) {
            return JS_ThrowReferenceError(ctx, "Unknown audio player id");
        }

        if (audioState->attached) {
            return JS_ThrowTypeError(ctx,
                                     "AudioPlayer is already attached to an object");
        }

        object->addComponent(audioState->component);
        runtime::scripting::registerComponentInstance(
            ctx, *host, audioState->component.get(),
            static_cast<int>(ownerId), "AudioPlayer", argv[1]);
        audioState->attached = true;

        auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
        if (objectIt != host->objectCache.end()) {
            syncObjectWrapper(ctx, *host, *object);
        }

        return JS_DupValue(ctx, argv[1]);
    }

    std::string componentName = "Component";
    JSValue ctor = JS_GetPropertyStr(ctx, argv[1], "constructor");
    if (!JS_IsException(ctor) && !JS_IsUndefined(ctor)) {
        readStringProperty(ctx, ctor, "name", componentName);
    }
    JS_FreeValue(ctx, ctor);

    auto component = std::make_shared<HostedScriptComponent>(
        ctx, host, argv[1], componentName);
    object->addComponent(component);

    auto objectIt = host->objectCache.find(static_cast<int>(ownerId));
    if (objectIt != host->objectCache.end()) {
        syncObjectWrapper(ctx, *host, *object);
    }

    return JS_DupValue(ctx, argv[1]);
}

std::shared_ptr<CoreObject> ownCoreObject(ScriptHost &host,
                                          std::shared_ptr<CoreObject> object,
                                          bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(object);
    }
    host.objectStates[object->getId()] = {.object = object.get(),
                                          .attachedToWindow = attachedToWindow};
    return object;
}

JSValue jsCreateCoreObject(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto object = ownCoreObject(*host, std::make_shared<CoreObject>());
    applyCoreObject(ctx, *host, argv[0], *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsUpdateObject(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    if (auto *core = dynamic_cast<CoreObject *>(object); core != nullptr) {
        if (!applyCoreObject(ctx, *host, argv[0], *core)) {
            return JS_EXCEPTION;
        }
    } else {
        applyBaseObject(ctx, *host, argv[0], *object);
        attachObjectIfReady(*host, *object);
    }

    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitiveBox(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size3d size;
    if (!parsePosition3d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size3d");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createBox(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitivePlane(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size2d size;
    if (!parseSize2d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size2d");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createPlane(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitivePyramid(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    Size3d size;
    if (!parsePosition3d(ctx, argv[0], size)) {
        return JS_ThrowTypeError(ctx, "Expected Size3d");
    }

    auto object = ownCoreObject(*host,
                                std::make_shared<CoreObject>(createPyramid(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitiveSphere(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected radius, sectorCount, and stackCount");
    }

    double radius = 0.0;
    std::int64_t sectorCount = 0;
    std::int64_t stackCount = 0;
    if (!getDouble(ctx, argv[0], radius) || !getInt64(ctx, argv[1], sectorCount) ||
        !getInt64(ctx, argv[2], stackCount)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected radius, sectorCount, and stackCount");
    }

    auto object = ownCoreObject(
        *host, std::make_shared<CoreObject>(createSphere(
                   radius, static_cast<unsigned int>(sectorCount),
                   static_cast<unsigned int>(stackCount))));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCloneCoreObject(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected CoreObject");
    }

    CoreObject *object = resolveCoreObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    auto clone =
        ownCoreObject(*host, std::make_shared<CoreObject>(object->clone()));
    attachObjectIfReady(*host, *clone);
    return syncObjectWrapper(ctx, *host, *clone);
}

JSValue jsMakeEmissive(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected id, color, intensity");
    }

    std::int64_t objectId = 0;
    double intensity = 0.0;
    if (!getInt64(ctx, argv[0], objectId) || !getDouble(ctx, argv[2], intensity)) {
        return JS_ThrowTypeError(ctx, "Expected id, color, intensity");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    Color color = Color::white();
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected Color");
    }

    object->makeEmissive(host->context->scene.get(), color,
                         static_cast<float>(intensity));
    attachObjectIfReady(*host, *object);
    return JS_UNDEFINED;
}

JSValue jsShowObject(JSContext *ctx, JSValueConst, int argc,
                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object != nullptr) {
        object->show();
    }
    return JS_UNDEFINED;
}

JSValue jsHideObject(JSContext *ctx, JSValueConst, int argc,
                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    GameObject *object = findObjectById(*host, static_cast<int>(objectId));
    if (object != nullptr) {
        object->hide();
    }
    return JS_UNDEFINED;
}

JSValue jsEnableDeferred(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }
    object->useDeferredRendering = true;
    return JS_UNDEFINED;
}

JSValue jsDisableDeferred(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_UNDEFINED;
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }
    object->disableDeferredRendering();
    return JS_UNDEFINED;
}

JSValue jsCreateInstance(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    object->createInstance();
    return syncInstanceWrapper(ctx, *host, *object,
                               static_cast<std::uint32_t>(object->instances.size() - 1));
}

JSValue jsCommitInstance(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected instance");
    }

    Instance *instance = resolveInstanceArg(ctx, *host, argv[0]);
    if (instance == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position = instance->position;
    Rotation3d rotation = instance->rotation;
    Scale3d scale = instance->scale;

    JSValue value = JS_GetPropertyStr(ctx, argv[0], "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, argv[0], "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, argv[0], "scale");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, scale);
    }
    JS_FreeValue(ctx, value);

    instance->setPosition(position);
    instance->setRotation(rotation);
    instance->setScale(scale);

    return JS_DupValue(ctx, argv[0]);
}

JSValue jsLookAtObject(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object and target");
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    Position3d up(0.0, 1.0, 0.0);
    if (argc > 2) {
        parsePosition3d(ctx, argv[2], up);
    }

    object->lookAt(target, up);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsSetRotationQuaternion(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object id and quaternion");
    }

    std::int64_t objectId = 0;
    if (!getInt64(ctx, argv[0], objectId)) {
        return JS_ThrowTypeError(ctx, "Expected object id");
    }

    auto *object = dynamic_cast<CoreObject *>(
        findObjectById(*host, static_cast<int>(objectId)));
    if (object == nullptr) {
        return JS_ThrowReferenceError(ctx, "Unknown CoreObject id");
    }

    glm::quat quat;
    if (!parseQuaternion(ctx, argv[1], quat)) {
        return JS_ThrowTypeError(ctx, "Expected Quaternion");
    }

    object->setRotationQuat(quat);
    return syncObjectWrapper(ctx, *host, *object);
}

} // namespace

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

bool runtime::scripting::checkNotException(JSContext *ctx, JSValueConst value,
                                           const char *what) {
    if (JS_IsException(value)) {
        std::cout << BOLD << RED << "Error during " << what << ": " << RESET;
        dumpExecution(ctx);
        return false;
    }
    return true;
}

void runtime::scripting::clearSceneBindings(JSContext *ctx, ScriptHost &host) {
    for (auto &[_, value] : host.objectCache) {
        JS_FreeValue(ctx, value);
    }
    host.objectCache.clear();

    for (auto &[_, value] : host.instanceCache) {
        JS_FreeValue(ctx, value);
    }
    host.instanceCache.clear();

    for (auto &[_, state] : host.componentStates) {
        JS_FreeValue(ctx, state.value);
    }
    host.componentStates.clear();

    for (auto &[_, state] : host.audioPlayers) {
        JS_FreeValue(ctx, state.value);
    }
    host.audioPlayers.clear();

    host.componentIds.clear();
    host.componentOrder.clear();
    host.componentLookup.clear();
    host.objectStates.clear();
    host.nextComponentId = 1;
    host.nextAudioPlayerId = 1;
    host.generation += 1;
}

std::uint64_t runtime::scripting::registerComponentInstance(
    JSContext *ctx, ScriptHost &host, Component *component, int ownerId,
    const std::string &name, JSValueConst value) {
    if (component == nullptr) {
        return 0;
    }

    std::uint64_t componentId = 0;
    auto existing = host.componentIds.find(component);
    if (existing != host.componentIds.end()) {
        componentId = existing->second;
        auto stateIt = host.componentStates.find(componentId);
        if (stateIt != host.componentStates.end()) {
            JS_FreeValue(ctx, stateIt->second.value);
        }
    } else {
        componentId = host.nextComponentId++;
        host.componentIds[component] = componentId;
        host.componentOrder[ownerId].push_back(componentId);
    }

    host.componentLookup[makeComponentLookupKey(ownerId, name)] = componentId;
    host.componentStates[componentId] = {.component = component,
                                         .ownerId = ownerId,
                                         .name = name,
                                         .value = JS_DupValue(ctx, value)};

    JSValue objectValue = JS_DupValue(ctx, value);
    setProperty(ctx, objectValue, ATLAS_COMPONENT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(componentId)));
    setProperty(ctx, objectValue, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, objectValue, "parentId", JS_NewInt32(ctx, ownerId));
    JS_FreeValue(ctx, objectValue);

    return componentId;
}

void runtime::scripting::installGlobals(JSContext *ctx) {
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "print",
                      JS_NewCFunction(ctx, jsPrint, "print", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetObjectById",
                      JS_NewCFunction(ctx, jsGetObjectById,
                                      "__atlasGetObjectById", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetObjectByName",
                      JS_NewCFunction(ctx, jsGetObjectByName,
                                      "__atlasGetObjectByName", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetComponentByName",
                      JS_NewCFunction(ctx, jsGetComponentByName,
                                      "__atlasGetComponentByName", 2));
    JS_SetPropertyStr(ctx, global, "__atlasLoadResource",
                      JS_NewCFunction(ctx, jsLoadResource, "__atlasLoadResource",
                                      3));
    JS_SetPropertyStr(ctx, global, "__atlasGetResourceByName",
                      JS_NewCFunction(ctx, jsGetResourceByName,
                                      "__atlasGetResourceByName", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAddComponent",
                      JS_NewCFunction(ctx, jsAddComponent, "__atlasAddComponent",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateAudioPlayer",
                      JS_NewCFunction(ctx, jsCreateAudioPlayer,
                                      "__atlasCreateAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasInitAudioPlayer",
                      JS_NewCFunction(ctx, jsInitAudioPlayer,
                                      "__atlasInitAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasPlayAudioPlayer",
                      JS_NewCFunction(ctx, jsPlayAudioPlayer,
                                      "__atlasPlayAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasPauseAudioPlayer",
                      JS_NewCFunction(ctx, jsPauseAudioPlayer,
                                      "__atlasPauseAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasStopAudioPlayer",
                      JS_NewCFunction(ctx, jsStopAudioPlayer,
                                      "__atlasStopAudioPlayer", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerVolume",
                      JS_NewCFunction(ctx, jsSetAudioPlayerVolume,
                                      "__atlasSetAudioPlayerVolume", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerLoop",
                      JS_NewCFunction(ctx, jsSetAudioPlayerLoop,
                                      "__atlasSetAudioPlayerLoop", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerSource",
                      JS_NewCFunction(ctx, jsSetAudioPlayerSource,
                                      "__atlasSetAudioPlayerSource", 2));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateAudioPlayer",
                      JS_NewCFunction(ctx, jsUpdateAudioPlayer,
                                      "__atlasUpdateAudioPlayer", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetAudioPlayerPosition",
                      JS_NewCFunction(ctx, jsSetAudioPlayerPosition,
                                      "__atlasSetAudioPlayerPosition", 2));
    JS_SetPropertyStr(ctx, global, "__atlasUseSpatialAudio",
                      JS_NewCFunction(ctx, jsUseSpatialAudio,
                                      "__atlasUseSpatialAudio", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateCoreObject",
                      JS_NewCFunction(ctx, jsCreateCoreObject,
                                      "__atlasCreateCoreObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateObject",
                      JS_NewCFunction(ctx, jsUpdateObject, "__atlasUpdateObject",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateCoreObject",
                      JS_NewCFunction(ctx, jsUpdateObject,
                                      "__atlasUpdateCoreObject", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateBox",
                      JS_NewCFunction(ctx, jsCreatePrimitiveBox,
                                      "__atlasCreateBox", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePlane",
                      JS_NewCFunction(ctx, jsCreatePrimitivePlane,
                                      "__atlasCreatePlane", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePyramid",
                      JS_NewCFunction(ctx, jsCreatePrimitivePyramid,
                                      "__atlasCreatePyramid", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSphere",
                      JS_NewCFunction(ctx, jsCreatePrimitiveSphere,
                                      "__atlasCreateSphere", 3));
    JS_SetPropertyStr(ctx, global, "__atlasCloneCoreObject",
                      JS_NewCFunction(ctx, jsCloneCoreObject,
                                      "__atlasCloneCoreObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMakeEmissive",
                      JS_NewCFunction(ctx, jsMakeEmissive,
                                      "__atlasMakeEmissive", 3));
    JS_SetPropertyStr(ctx, global, "__atlasShowObject",
                      JS_NewCFunction(ctx, jsShowObject, "__atlasShowObject",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasHideObject",
                      JS_NewCFunction(ctx, jsHideObject, "__atlasHideObject",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasShowCoreObject",
                      JS_NewCFunction(ctx, jsShowObject,
                                      "__atlasShowCoreObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasHideCoreObject",
                      JS_NewCFunction(ctx, jsHideObject,
                                      "__atlasHideCoreObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasEnableDeferredRendering",
                      JS_NewCFunction(ctx, jsEnableDeferred,
                                      "__atlasEnableDeferredRendering", 1));
    JS_SetPropertyStr(ctx, global, "__atlasDisableDeferredRendering",
                      JS_NewCFunction(ctx, jsDisableDeferred,
                                      "__atlasDisableDeferredRendering", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateInstance",
                      JS_NewCFunction(ctx, jsCreateInstance,
                                      "__atlasCreateInstance", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCommitInstance",
                      JS_NewCFunction(ctx, jsCommitInstance,
                                      "__atlasCommitInstance", 1));
    JS_SetPropertyStr(ctx, global, "__atlasLookAtObject",
                      JS_NewCFunction(ctx, jsLookAtObject,
                                      "__atlasLookAtObject", 3));
    JS_SetPropertyStr(ctx, global, "__atlasSetRotationQuaternion",
                      JS_NewCFunction(ctx, jsSetRotationQuaternion,
                                      "__atlasSetRotationQuaternion", 2));
    JS_FreeValue(ctx, global);
}

char *runtime::scripting::normalizeModuleName(JSContext *ctx,
                                              const char *baseName,
                                              const char *name,
                                              void *opaque) {
    auto *host = static_cast<ScriptHost *>(opaque);
    const std::string base = baseName == nullptr ? "" : baseName;
    const std::string module = name == nullptr ? "" : name;

    if (host->modules.contains(module)) {
        return js_strdup(ctx, module.c_str());
    }

    if (!module.empty() && module[0] == '.') {
        auto slash = base.rfind('/');
        std::string dir =
            (slash == std::string::npos) ? "" : base.substr(0, slash + 1);
        std::string resolved = dir + module;

        while (true) {
            auto pos = resolved.find("/./");
            if (pos == std::string::npos) {
                break;
            }
            resolved.replace(pos, 3, "/");
        }

        while (true) {
            auto pos = resolved.find("../");
            if (pos == std::string::npos) {
                break;
            }
            auto prev = resolved.rfind('/', pos > 1 ? pos - 2 : 0);
            if (prev == std::string::npos) {
                break;
            }
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
                           module.c_str(),
                           base.empty() ? "<root>" : base.c_str());
    return nullptr;
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

bool runtime::scripting::evalModule(JSContext *ctx, const std::string &name,
                                    const std::string &src) {
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
                                                  const std::string &module_name) {
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

bool ScriptInstance::callMethod(const char *method_name, int argc,
                                JSValueConst *argv) {
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

ScriptInstance *runtime::scripting::createScriptInstance(
    JSContext *ctx, const std::string &entryModuleName,
    const std::string &scriptPath, const std::string &className) {
    JSValue ns =
        runtime::scripting::importModuleNamespace(ctx, entryModuleName);
    if (JS_IsException(ns)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    JSValue script_exports = JS_UNDEFINED;
    if (scriptPath.empty()) {
        script_exports = ns;
    } else {
        JSValue atlas_scripts = JS_GetPropertyStr(ctx, ns, "default");
        JS_FreeValue(ctx, ns);
        if (JS_IsException(atlas_scripts)) {
            runtime::scripting::dumpExecution(ctx);
            return nullptr;
        }

        script_exports =
            JS_GetPropertyStr(ctx, atlas_scripts, scriptPath.c_str());
        JS_FreeValue(ctx, atlas_scripts);
        if (JS_IsException(script_exports)) {
            runtime::scripting::dumpExecution(ctx);
            return nullptr;
        }

        if (JS_IsUndefined(script_exports)) {
            std::cerr << "Script exports not found for path: " << scriptPath
                      << "\n";
            JS_FreeValue(ctx, script_exports);
            return nullptr;
        }
    }

    JSValue ctor = JS_GetPropertyStr(ctx, script_exports, className.c_str());
    JS_FreeValue(ctx, script_exports);
    if (JS_IsException(ctor)) {
        runtime::scripting::dumpExecution(ctx);
        return nullptr;
    }

    if (!JS_IsFunction(ctx, ctor)) {
        std::cerr << "Export '" << className
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
