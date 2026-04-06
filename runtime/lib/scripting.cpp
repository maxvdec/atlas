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
#include "atlas/effect.h"
#include "atlas/input.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/particle.h"
#include "atlas/runtime/context.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
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
constexpr const char *ATLAS_TEXTURE_ID_PROP = "__atlasTextureId";
constexpr const char *ATLAS_CUBEMAP_ID_PROP = "__atlasCubemapId";
constexpr const char *ATLAS_SKYBOX_ID_PROP = "__atlasSkyboxId";
constexpr const char *ATLAS_RENDER_TARGET_ID_PROP = "__atlasRenderTargetId";
constexpr const char *ATLAS_POINT_LIGHT_ID_PROP = "__atlasPointLightId";
constexpr const char *ATLAS_DIRECTIONAL_LIGHT_ID_PROP =
    "__atlasDirectionalLightId";
constexpr const char *ATLAS_SPOT_LIGHT_ID_PROP = "__atlasSpotLightId";
constexpr const char *ATLAS_AREA_LIGHT_ID_PROP = "__atlasAreaLightId";
constexpr const char *ATLAS_PARTICLE_EMITTER_ID_PROP =
    "__atlasParticleEmitterId";
constexpr const char *ATLAS_GENERATION_PROP = "__atlasGeneration";
constexpr const char *ATLAS_IS_CORE_OBJECT_PROP = "__atlasIsCoreObject";
constexpr const char *ATLAS_CAMERA_PROP = "__atlasCamera";
constexpr const char *ATLAS_SCENE_PROP = "__atlasScene";
constexpr const char *ATLAS_NATIVE_COMPONENT_KIND_PROP =
    "__atlasNativeComponentKind";

const auto ATLAS_KEY_ENTRIES = std::to_array<std::pair<const char *, Key>>({
    {"Unknown", Key::Unknown},
    {"Space", Key::Space},
    {"Apostrophe", Key::Apostrophe},
    {"Comma", Key::Comma},
    {"Minus", Key::Minus},
    {"Period", Key::Period},
    {"Slash", Key::Slash},
    {"Key0", Key::Key0},
    {"Key1", Key::Key1},
    {"Key2", Key::Key2},
    {"Key3", Key::Key3},
    {"Key4", Key::Key4},
    {"Key5", Key::Key5},
    {"Key6", Key::Key6},
    {"Key7", Key::Key7},
    {"Key8", Key::Key8},
    {"Key9", Key::Key9},
    {"Semicolon", Key::Semicolon},
    {"Equal", Key::Equal},
    {"A", Key::A},
    {"B", Key::B},
    {"C", Key::C},
    {"D", Key::D},
    {"E", Key::E},
    {"F", Key::F},
    {"G", Key::G},
    {"H", Key::H},
    {"I", Key::I},
    {"J", Key::J},
    {"K", Key::K},
    {"L", Key::L},
    {"M", Key::M},
    {"N", Key::N},
    {"O", Key::O},
    {"P", Key::P},
    {"Q", Key::Q},
    {"R", Key::R},
    {"S", Key::S},
    {"T", Key::T},
    {"U", Key::U},
    {"V", Key::V},
    {"W", Key::W},
    {"X", Key::X},
    {"Y", Key::Y},
    {"Z", Key::Z},
    {"LeftBracket", Key::LeftBracket},
    {"Backslash", Key::Backslash},
    {"RightBracket", Key::RightBracket},
    {"GraveAccent", Key::GraveAccent},
    {"Escape", Key::Escape},
    {"Enter", Key::Enter},
    {"Tab", Key::Tab},
    {"Backspace", Key::Backspace},
    {"Insert", Key::Insert},
    {"Delete", Key::Delete},
    {"Right", Key::Right},
    {"Left", Key::Left},
    {"Down", Key::Down},
    {"Up", Key::Up},
    {"PageUp", Key::PageUp},
    {"PageDown", Key::PageDown},
    {"Home", Key::Home},
    {"End", Key::End},
    {"CapsLock", Key::CapsLock},
    {"ScrollLock", Key::ScrollLock},
    {"NumLock", Key::NumLock},
    {"PrintScreen", Key::PrintScreen},
    {"Pause", Key::Pause},
    {"F1", Key::F1},
    {"F2", Key::F2},
    {"F3", Key::F3},
    {"F4", Key::F4},
    {"F5", Key::F5},
    {"F6", Key::F6},
    {"F7", Key::F7},
    {"F8", Key::F8},
    {"F9", Key::F9},
    {"F10", Key::F10},
    {"F11", Key::F11},
    {"F12", Key::F12},
    {"F13", Key::F13},
    {"F14", Key::F14},
    {"F15", Key::F15},
    {"F16", Key::F16},
    {"F17", Key::F17},
    {"F18", Key::F18},
    {"F19", Key::F19},
    {"F20", Key::F20},
    {"F21", Key::F21},
    {"F22", Key::F22},
    {"F23", Key::F23},
    {"F24", Key::F24},
    {"F25", Key::F25},
    {"KP0", Key::KP0},
    {"KP1", Key::KP1},
    {"KP2", Key::KP2},
    {"KP3", Key::KP3},
    {"KP4", Key::KP4},
    {"KP5", Key::KP5},
    {"KP6", Key::KP6},
    {"KP7", Key::KP7},
    {"KP8", Key::KP8},
    {"KP9", Key::KP9},
    {"KPDecimal", Key::KPDecimal},
    {"KPDivide", Key::KPDivide},
    {"KPMultiply", Key::KPMultiply},
    {"KPSubtract", Key::KPSubtract},
    {"KPAdd", Key::KPAdd},
    {"KPEnter", Key::KPEnter},
    {"KPEqual", Key::KPEqual},
    {"LeftShift", Key::LeftShift},
    {"LeftControl", Key::LeftControl},
    {"LeftAlt", Key::LeftAlt},
    {"LeftSuper", Key::LeftSuper},
    {"RightShift", Key::RightShift},
    {"RightControl", Key::RightControl},
    {"RightAlt", Key::RightAlt},
    {"RightSuper", Key::RightSuper},
    {"Menu", Key::Menu},
});

const auto ATLAS_MOUSE_BUTTON_ENTRIES =
    std::to_array<std::pair<const char *, MouseButton>>({
        {"Left", MouseButton::Left},
        {"Right", MouseButton::Right},
        {"Middle", MouseButton::Middle},
        {"X1", MouseButton::Button4},
        {"X2", MouseButton::Button5},
        {"Button4", MouseButton::Button4},
        {"Button5", MouseButton::Button5},
        {"Button6", MouseButton::Button6},
        {"Button7", MouseButton::Button7},
        {"Button8", MouseButton::Button8},
        {"Last", MouseButton::Last},
    });

const std::array<MouseButton, 8> ATLAS_MOUSE_BUTTONS = {
    MouseButton::Left,    MouseButton::Right,   MouseButton::Middle,
    MouseButton::Button4, MouseButton::Button5, MouseButton::Button6,
    MouseButton::Button7, MouseButton::Button8};

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

TextureType toNativeTextureType(int type) {
    switch (type) {
    case 1:
        return TextureType::Specular;
    case 2:
        return TextureType::Cubemap;
    case 3:
        return TextureType::Depth;
    case 4:
        return TextureType::DepthCube;
    case 5:
        return TextureType::Normal;
    case 6:
        return TextureType::Parallax;
    case 7:
        return TextureType::SSAONoise;
    case 8:
        return TextureType::SSAO;
    case 9:
        return TextureType::Metallic;
    case 10:
        return TextureType::Roughness;
    case 11:
        return TextureType::AO;
    case 12:
        return TextureType::Opacity;
    case 13:
        return TextureType::HDR;
    case 0:
    default:
        return TextureType::Color;
    }
}

int toScriptTextureType(TextureType type) {
    switch (type) {
    case TextureType::Specular:
        return 1;
    case TextureType::Cubemap:
        return 2;
    case TextureType::Depth:
        return 3;
    case TextureType::DepthCube:
        return 4;
    case TextureType::Normal:
        return 5;
    case TextureType::Parallax:
        return 6;
    case TextureType::SSAONoise:
        return 7;
    case TextureType::SSAO:
        return 8;
    case TextureType::Metallic:
        return 9;
    case TextureType::Roughness:
        return 10;
    case TextureType::AO:
        return 11;
    case TextureType::Opacity:
        return 12;
    case TextureType::HDR:
        return 13;
    case TextureType::Color:
    default:
        return 0;
    }
}

RenderTargetType toNativeRenderTargetType(int type) {
    switch (type) {
    case 1:
        return RenderTargetType::Multisampled;
    case 2:
        return RenderTargetType::Shadow;
    case 3:
        return RenderTargetType::CubeShadow;
    case 4:
        return RenderTargetType::GBuffer;
    case 5:
        return RenderTargetType::SSAO;
    case 6:
        return RenderTargetType::SSAOBlur;
    case 0:
    default:
        return RenderTargetType::Scene;
    }
}

int toScriptRenderTargetType(RenderTargetType type) {
    switch (type) {
    case RenderTargetType::Multisampled:
        return 1;
    case RenderTargetType::Shadow:
        return 2;
    case RenderTargetType::CubeShadow:
        return 3;
    case RenderTargetType::GBuffer:
        return 4;
    case RenderTargetType::SSAO:
        return 5;
    case RenderTargetType::SSAOBlur:
        return 6;
    case RenderTargetType::Scene:
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

Camera *getSceneCamera(ScriptHost &host) {
    if (host.context == nullptr) {
        return nullptr;
    }
    if (host.context->camera != nullptr) {
        return host.context->camera.get();
    }
    if (host.context->window != nullptr) {
        return host.context->window->getCamera();
    }
    return nullptr;
}

Scene *getScene(ScriptHost &host) {
    if (host.context == nullptr) {
        return nullptr;
    }
    if (host.context->window != nullptr) {
        Scene *currentScene = host.context->window->getCurrentScene();
        if (currentScene != nullptr) {
            return currentScene;
        }
    }
    return host.context->scene.get();
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

    if (JS_IsUndefined(host.atlasGraphicsNamespace)) {
        host.atlasGraphicsNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/graphics");
        if (JS_IsException(host.atlasGraphicsNamespace)) {
            runtime::scripting::dumpExecution(ctx);
            return false;
        }
    }

    if (JS_IsUndefined(host.atlasParticleNamespace)) {
        host.atlasParticleNamespace =
            runtime::scripting::importModuleNamespace(ctx, "atlas/particles");
        if (JS_IsException(host.atlasParticleNamespace)) {
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
    cachePrototype(ctx, host.atlasNamespace, "Model", host.modelPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Material",
                   host.materialPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Instance",
                   host.instancePrototype);
    cachePrototype(ctx, host.atlasNamespace, "CoreVertex",
                   host.coreVertexPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Resource",
                   host.resourcePrototype);
    cachePrototype(ctx, host.atlasNamespace, "Camera", host.cameraPrototype);
    cachePrototype(ctx, host.atlasNamespace, "Scene", host.scenePrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Texture",
                   host.texturePrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Cubemap",
                   host.cubemapPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Skybox",
                   host.skyboxPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "RenderTarget",
                   host.renderTargetPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "Light",
                   host.pointLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "DirectionalLight",
                   host.directionalLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "SpotLight",
                   host.spotLightPrototype);
    cachePrototype(ctx, host.atlasGraphicsNamespace, "AreaLight",
                   host.areaLightPrototype);
    cachePrototype(ctx, host.atlasParticleNamespace, "ParticleEmitter",
                   host.particleEmitterPrototype);
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

JSValue makeSize2d(JSContext *ctx, ScriptHost &host, const Size2d &value) {
    JSValue result = newObjectFromPrototype(ctx, host.size2dPrototype);
    setProperty(ctx, result, "width", JS_NewFloat64(ctx, value.width));
    setProperty(ctx, result, "height", JS_NewFloat64(ctx, value.height));
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

JSValue makeResource(JSContext *ctx, ScriptHost &host,
                     const Resource &resource) {
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

bool parsePosition2d(JSContext *ctx, JSValueConst value, Position2d &out) {
    double x = 0.0;
    double y = 0.0;
    if (!readNumberProperty(ctx, value, "x", x) ||
        !readNumberProperty(ctx, value, "y", y)) {
        return false;
    }
    out = Position2d(x, y);
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
    out =
        glm::normalize(glm::quat(static_cast<float>(w), static_cast<float>(x),
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

JSValue makeParticleSettings(JSContext *ctx, const ParticleSettings &settings) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "minLifetime",
                JS_NewFloat64(ctx, settings.minLifetime));
    setProperty(ctx, result, "maxLifetime",
                JS_NewFloat64(ctx, settings.maxLifetime));
    setProperty(ctx, result, "minSize", JS_NewFloat64(ctx, settings.minSize));
    setProperty(ctx, result, "maxSize", JS_NewFloat64(ctx, settings.maxSize));
    setProperty(ctx, result, "fadeSpeed",
                JS_NewFloat64(ctx, settings.fadeSpeed));
    setProperty(ctx, result, "gravity", JS_NewFloat64(ctx, settings.gravity));
    setProperty(ctx, result, "spread", JS_NewFloat64(ctx, settings.spread));
    setProperty(ctx, result, "speedVariation",
                JS_NewFloat64(ctx, settings.speedVariation));
    return result;
}

bool parseParticleSettings(JSContext *ctx, JSValueConst value,
                           ParticleSettings &out) {
    double minLifetime = out.minLifetime;
    double maxLifetime = out.maxLifetime;
    double minSize = out.minSize;
    double maxSize = out.maxSize;
    double fadeSpeed = out.fadeSpeed;
    double gravity = out.gravity;
    double spread = out.spread;
    double speedVariation = out.speedVariation;

    readNumberProperty(ctx, value, "minLifetime", minLifetime);
    readNumberProperty(ctx, value, "maxLifetime", maxLifetime);
    readNumberProperty(ctx, value, "minSize", minSize);
    readNumberProperty(ctx, value, "maxSize", maxSize);
    readNumberProperty(ctx, value, "fadeSpeed", fadeSpeed);
    readNumberProperty(ctx, value, "gravity", gravity);
    readNumberProperty(ctx, value, "spread", spread);
    readNumberProperty(ctx, value, "speedVariation", speedVariation);

    out.minLifetime = static_cast<float>(minLifetime);
    out.maxLifetime = static_cast<float>(maxLifetime);
    out.minSize = static_cast<float>(minSize);
    out.maxSize = static_cast<float>(maxSize);
    out.fadeSpeed = static_cast<float>(fadeSpeed);
    out.gravity = static_cast<float>(gravity);
    out.spread = static_cast<float>(spread);
    out.speedVariation = static_cast<float>(speedVariation);
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

bool getArrayLength(JSContext *ctx, JSValueConst value, std::uint32_t &length) {
    JSValue lengthValue = JS_GetPropertyStr(ctx, value, "length");
    if (JS_IsException(lengthValue)) {
        return false;
    }
    const bool ok = getUint32(ctx, lengthValue, length);
    JS_FreeValue(ctx, lengthValue);
    return ok;
}

bool parseColorArray(JSContext *ctx, JSValueConst value,
                     std::array<Color, 6> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    if (!getArrayLength(ctx, value, length) || length != out.size()) {
        return false;
    }

    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item) || !parseColor(ctx, item, out[i])) {
            JS_FreeValue(ctx, item);
            return false;
        }
        JS_FreeValue(ctx, item);
    }

    return true;
}

bool parseResourceVector(JSContext *ctx, JSValueConst value,
                         std::vector<Resource> &out) {
    if (!JS_IsArray(value)) {
        return false;
    }

    std::uint32_t length = 0;
    if (!getArrayLength(ctx, value, length)) {
        return false;
    }

    out.clear();
    out.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
        JSValue item = JS_GetPropertyUint32(ctx, value, i);
        if (JS_IsException(item)) {
            JS_FreeValue(ctx, item);
            return false;
        }
        Resource resource;
        const bool ok = parseResource(ctx, item, resource);
        JS_FreeValue(ctx, item);
        if (!ok) {
            return false;
        }
        out.push_back(resource);
    }

    return true;
}

JSValue makePlainPosition2d(JSContext *ctx, const Position2d &value) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, value.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, value.y));
    return result;
}

JSValue makeAxisPacketValue(JSContext *ctx, const AxisPacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "deltaX", JS_NewFloat64(ctx, packet.deltaX));
    setProperty(ctx, result, "deltaY", JS_NewFloat64(ctx, packet.deltaY));
    setProperty(ctx, result, "x", JS_NewFloat64(ctx, packet.x));
    setProperty(ctx, result, "y", JS_NewFloat64(ctx, packet.y));
    setProperty(ctx, result, "valueX", JS_NewFloat64(ctx, packet.valueX));
    setProperty(ctx, result, "valueY", JS_NewFloat64(ctx, packet.valueY));
    setProperty(ctx, result, "inputDeltaX",
                JS_NewFloat64(ctx, packet.inputDeltaX));
    setProperty(ctx, result, "inputDeltaY",
                JS_NewFloat64(ctx, packet.inputDeltaY));
    setProperty(ctx, result, "hasValueInput",
                JS_NewBool(ctx, packet.hasValueInput));
    setProperty(ctx, result, "hasDeltaInput",
                JS_NewBool(ctx, packet.hasDeltaInput));
    return result;
}

JSValue makeMousePacketValue(JSContext *ctx, const MousePacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "xpos", JS_NewFloat64(ctx, packet.xpos));
    setProperty(ctx, result, "ypos", JS_NewFloat64(ctx, packet.ypos));
    setProperty(ctx, result, "xoffset", JS_NewFloat64(ctx, packet.xoffset));
    setProperty(ctx, result, "yoffset", JS_NewFloat64(ctx, packet.yoffset));
    setProperty(ctx, result, "constrainPitch",
                JS_NewBool(ctx, packet.constrainPitch));
    setProperty(ctx, result, "firstMouse", JS_NewBool(ctx, packet.firstMouse));
    return result;
}

JSValue makeMouseScrollPacketValue(JSContext *ctx,
                                   const MouseScrollPacket &packet) {
    JSValue result = JS_NewObject(ctx);
    setProperty(ctx, result, "xoffset", JS_NewFloat64(ctx, packet.xoffset));
    setProperty(ctx, result, "yoffset", JS_NewFloat64(ctx, packet.yoffset));
    return result;
}

bool parseTrigger(JSContext *ctx, JSValueConst value, Trigger &out) {
    std::int64_t type = 0;
    if (!readIntProperty(ctx, value, "type", type)) {
        return false;
    }

    switch (static_cast<TriggerType>(type)) {
    case TriggerType::MouseButton: {
        std::int64_t button = 0;
        if (!readIntProperty(ctx, value, "mouseButton", button)) {
            return false;
        }
        out = Trigger::fromMouseButton(static_cast<MouseButton>(button));
        return true;
    }
    case TriggerType::Key: {
        std::int64_t key = 0;
        if (!readIntProperty(ctx, value, "key", key)) {
            return false;
        }
        out = Trigger::fromKey(static_cast<Key>(key));
        return true;
    }
    case TriggerType::ControllerButton: {
        JSValue controllerButton =
            JS_GetPropertyStr(ctx, value, "controllerButton");
        if (JS_IsException(controllerButton) ||
            JS_IsUndefined(controllerButton)) {
            JS_FreeValue(ctx, controllerButton);
            return false;
        }
        std::int64_t controllerID = 0;
        std::int64_t buttonIndex = 0;
        const bool ok =
            readIntProperty(ctx, controllerButton, "controllerID",
                            controllerID) &&
            readIntProperty(ctx, controllerButton, "buttonIndex", buttonIndex);
        JS_FreeValue(ctx, controllerButton);
        if (!ok) {
            return false;
        }
        out = Trigger::fromControllerButton(static_cast<int>(controllerID),
                                            static_cast<int>(buttonIndex));
        return true;
    }
    }

    return false;
}

bool parseAxisTrigger(JSContext *ctx, JSValueConst value, AxisTrigger &out) {
    std::int64_t type = 0;
    if (!readIntProperty(ctx, value, "type", type)) {
        return false;
    }

    switch (static_cast<AxisTriggerType>(type)) {
    case AxisTriggerType::MouseAxis:
        out = AxisTrigger::mouse();
        break;
    case AxisTriggerType::KeyCustom: {
        Trigger positiveX;
        Trigger negativeX;
        Trigger positiveY;
        Trigger negativeY;

        JSValue prop = JS_GetPropertyStr(ctx, value, "positiveX");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, positiveX)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "negativeX");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, negativeX)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "positiveY");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, positiveY)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        prop = JS_GetPropertyStr(ctx, value, "negativeY");
        if (JS_IsException(prop) || !parseTrigger(ctx, prop, negativeY)) {
            JS_FreeValue(ctx, prop);
            return false;
        }
        JS_FreeValue(ctx, prop);

        out = AxisTrigger::custom(positiveX, negativeX, positiveY, negativeY);
        break;
    }
    case AxisTriggerType::ControllerAxis: {
        std::int64_t controllerId = CONTROLLER_UNDEFINED;
        std::int64_t axisIndex = -1;
        std::int64_t axisIndexY = -1;
        bool single = false;

        readIntProperty(ctx, value, "controllerId", controllerId);
        readIntProperty(ctx, value, "axisIndex", axisIndex);
        readIntProperty(ctx, value, "axisIndexY", axisIndexY);
        readBoolProperty(ctx, value, "controllerAxisSingle", single);

        if (axisIndex < 0) {
            return false;
        }

        out = AxisTrigger::controller(static_cast<int>(controllerId),
                                      static_cast<int>(axisIndex), single,
                                      static_cast<int>(axisIndexY));
        break;
    }
    }

    bool isJoystick = out.isJoystick;
    readBoolProperty(ctx, value, "isJoystick", isJoystick);
    out.isJoystick = isJoystick;
    return true;
}

bool parseInputAction(JSContext *ctx, JSValueConst value, InputAction &out) {
    std::string name;
    if (!readStringProperty(ctx, value, "name", name)) {
        return false;
    }

    bool isAxis = false;
    readBoolProperty(ctx, value, "isAxis", isAxis);
    bool isAxisSingle = false;
    readBoolProperty(ctx, value, "isAxisSingle", isAxisSingle);

    out = InputAction();
    out.name = name;
    out.isAxis = isAxis;
    out.isAxisSingle = isAxisSingle;

    if (isAxis) {
        JSValue axisTriggers = JS_GetPropertyStr(ctx, value, "axisTriggers");
        if (!JS_IsException(axisTriggers) && JS_IsArray(axisTriggers)) {
            std::uint32_t length = 0;
            if (!getArrayLength(ctx, axisTriggers, length)) {
                JS_FreeValue(ctx, axisTriggers);
                return false;
            }
            out.axisTriggers.clear();
            out.axisTriggers.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue axisTriggerValue =
                    JS_GetPropertyUint32(ctx, axisTriggers, i);
                AxisTrigger axisTrigger;
                const bool ok =
                    !JS_IsException(axisTriggerValue) &&
                    parseAxisTrigger(ctx, axisTriggerValue, axisTrigger);
                JS_FreeValue(ctx, axisTriggerValue);
                if (!ok) {
                    JS_FreeValue(ctx, axisTriggers);
                    return false;
                }
                out.axisTriggers.push_back(axisTrigger);
            }
        }
        JS_FreeValue(ctx, axisTriggers);
    } else {
        JSValue triggers = JS_GetPropertyStr(ctx, value, "triggers");
        if (!JS_IsException(triggers) && JS_IsArray(triggers)) {
            std::uint32_t length = 0;
            if (!getArrayLength(ctx, triggers, length)) {
                JS_FreeValue(ctx, triggers);
                return false;
            }
            out.buttonTriggers.clear();
            out.buttonTriggers.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                JSValue triggerValue = JS_GetPropertyUint32(ctx, triggers, i);
                Trigger trigger;
                const bool ok = !JS_IsException(triggerValue) &&
                                parseTrigger(ctx, triggerValue, trigger);
                JS_FreeValue(ctx, triggerValue);
                if (!ok) {
                    JS_FreeValue(ctx, triggers);
                    return false;
                }
                out.buttonTriggers.push_back(trigger);
            }
        }
        JS_FreeValue(ctx, triggers);
    }

    bool normalized = out.normalize2D;
    bool invertY = out.invertControllerY;
    double controllerDeadzone = out.controllerDeadzone;
    readBoolProperty(ctx, value, "normalized", normalized);
    readBoolProperty(ctx, value, "invertY", invertY);
    readNumberProperty(ctx, value, "controllerDeadzone", controllerDeadzone);
    out.normalize2D = normalized;
    out.invertControllerY = invertY;
    out.controllerDeadzone = static_cast<float>(controllerDeadzone);
    return true;
}

bool callObjectMethod(JSContext *ctx, JSValueConst object,
                      const char *methodName, int argc, JSValueConst *argv) {
    JSValue fn = JS_GetPropertyStr(ctx, object, methodName);
    if (JS_IsException(fn)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }
    if (JS_IsUndefined(fn) || !JS_IsFunction(ctx, fn)) {
        JS_FreeValue(ctx, fn);
        return false;
    }

    JSValue ret = JS_Call(ctx, fn, object, argc, argv);
    JS_FreeValue(ctx, fn);
    if (JS_IsException(ret)) {
        runtime::scripting::dumpExecution(ctx);
        return false;
    }

    JS_FreeValue(ctx, ret);
    return true;
}

ScriptTextureState *findTextureState(ScriptHost &host,
                                     std::uint64_t textureId) {
    auto it = host.textures.find(textureId);
    if (it == host.textures.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptCubemapState *findCubemapState(ScriptHost &host,
                                     std::uint64_t cubemapId) {
    auto it = host.cubemaps.find(cubemapId);
    if (it == host.cubemaps.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptSkyboxState *findSkyboxState(ScriptHost &host, std::uint64_t skyboxId) {
    auto it = host.skyboxes.find(skyboxId);
    if (it == host.skyboxes.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptRenderTargetState *findRenderTargetState(ScriptHost &host,
                                               std::uint64_t renderTargetId) {
    auto it = host.renderTargets.find(renderTargetId);
    if (it == host.renderTargets.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptPointLightState *findPointLightState(ScriptHost &host,
                                           std::uint64_t lightId) {
    auto it = host.pointLights.find(lightId);
    if (it == host.pointLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptDirectionalLightState *findDirectionalLightState(ScriptHost &host,
                                                       std::uint64_t lightId) {
    auto it = host.directionalLights.find(lightId);
    if (it == host.directionalLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptSpotLightState *findSpotLightState(ScriptHost &host,
                                         std::uint64_t lightId) {
    auto it = host.spotLights.find(lightId);
    if (it == host.spotLights.end()) {
        return nullptr;
    }
    return &it->second;
}

ScriptAreaLightState *findAreaLightState(ScriptHost &host,
                                         std::uint64_t lightId) {
    auto it = host.areaLights.find(lightId);
    if (it == host.areaLights.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint64_t registerTextureState(ScriptHost &host, const Texture &texture) {
    const std::uint64_t textureId = host.nextTextureId++;
    host.textures[textureId] = {.texture = std::make_shared<Texture>(texture),
                                .value = JS_UNDEFINED};
    return textureId;
}

std::uint64_t registerCubemapState(ScriptHost &host, const Cubemap &cubemap) {
    const std::uint64_t cubemapId = host.nextCubemapId++;
    host.cubemaps[cubemapId] = {.cubemap = std::make_shared<Cubemap>(cubemap),
                                .value = JS_UNDEFINED};
    return cubemapId;
}

JSValue syncTextureWrapper(JSContext *ctx, ScriptHost &host,
                           std::uint64_t textureId) {
    auto *state = findTextureState(host, textureId);
    if (state == nullptr || !state->texture) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.texturePrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    const Resource resource = state->texture->resource;
    setProperty(ctx, wrapper, ATLAS_TEXTURE_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(textureId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "type",
                JS_NewInt32(ctx, toScriptTextureType(state->texture->type)));
    setProperty(ctx, wrapper, "resource", makeResource(ctx, host, resource));
    setProperty(ctx, wrapper, "width",
                JS_NewInt32(ctx, state->texture->creationData.width));
    setProperty(ctx, wrapper, "height",
                JS_NewInt32(ctx, state->texture->creationData.height));
    setProperty(ctx, wrapper, "channels",
                JS_NewInt32(ctx, state->texture->creationData.channels));
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(state->texture->id)));
    setProperty(ctx, wrapper, "borderColor",
                makeColor(ctx, host, state->texture->borderColor));
    return wrapper;
}

JSValue syncCubemapWrapper(JSContext *ctx, ScriptHost &host,
                           std::uint64_t cubemapId) {
    auto *state = findCubemapState(host, cubemapId);
    if (state == nullptr || !state->cubemap) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.cubemapPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    JSValue resources = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < state->cubemap->resources.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, resources, i,
            makeResource(ctx, host, state->cubemap->resources[i]));
    }

    setProperty(ctx, wrapper, ATLAS_CUBEMAP_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(cubemapId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "resources", resources);
    setProperty(ctx, wrapper, "id",
                JS_NewInt64(ctx, static_cast<int64_t>(state->cubemap->id)));
    return wrapper;
}

JSValue syncSkyboxWrapper(JSContext *ctx, ScriptHost &host,
                          std::uint64_t skyboxId) {
    auto *state = findSkyboxState(host, skyboxId);
    if (state == nullptr || !state->skybox) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.skyboxPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_SKYBOX_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(skyboxId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    if (state->cubemapId != 0) {
        setProperty(ctx, wrapper, "cubemap",
                    syncCubemapWrapper(ctx, host, state->cubemapId));
    }
    return wrapper;
}

void syncRenderTargetTextureStates(ScriptHost &host,
                                   std::uint64_t renderTargetId) {
    auto *state = findRenderTargetState(host, renderTargetId);
    if (state == nullptr || !state->renderTarget) {
        return;
    }

    auto assignTexture = [&](std::uint64_t &slot, const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            slot = 0;
            return;
        }
        if (slot == 0) {
            slot = registerTextureState(host, texture);
            return;
        }
        auto *textureState = findTextureState(host, slot);
        if (textureState != nullptr && textureState->texture) {
            *textureState->texture = texture;
        }
    };

    std::vector<std::uint64_t> currentOutTextureIds = state->outTextureIds;
    state->outTextureIds.clear();
    std::size_t outTextureIndex = 0;
    auto pushTexture = [&](const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            return;
        }
        std::uint64_t slot = 0;
        if (outTextureIndex < currentOutTextureIds.size()) {
            slot = currentOutTextureIds[outTextureIndex];
        }
        assignTexture(slot, texture);
        if (slot != 0) {
            state->outTextureIds.push_back(slot);
        }
        outTextureIndex += 1;
    };

    switch (state->renderTarget->type) {
    case RenderTargetType::GBuffer:
        pushTexture(state->renderTarget->gPosition);
        pushTexture(state->renderTarget->gNormal);
        pushTexture(state->renderTarget->gAlbedoSpec);
        pushTexture(state->renderTarget->gMaterial);
        break;
    case RenderTargetType::Shadow:
    case RenderTargetType::CubeShadow:
        break;
    default:
        pushTexture(state->renderTarget->texture);
        if (state->renderTarget->brightTexture.id != 0 ||
            state->renderTarget->brightTexture.texture != nullptr) {
            pushTexture(state->renderTarget->brightTexture);
        }
        break;
    }

    if (state->renderTarget->type == RenderTargetType::Shadow ||
        state->renderTarget->type == RenderTargetType::CubeShadow) {
        assignTexture(state->depthTextureId, state->renderTarget->texture);
    } else if (state->renderTarget->depthTexture.id != 0 ||
               state->renderTarget->depthTexture.texture != nullptr) {
        assignTexture(state->depthTextureId, state->renderTarget->depthTexture);
    } else {
        state->depthTextureId = 0;
    }
}

JSValue syncRenderTargetWrapper(JSContext *ctx, ScriptHost &host,
                                std::uint64_t renderTargetId) {
    auto *state = findRenderTargetState(host, renderTargetId);
    if (state == nullptr || !state->renderTarget) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    syncRenderTargetTextureStates(host, renderTargetId);

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.renderTargetPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    JSValue outTextures = JS_NewArray(ctx);
    for (std::uint32_t i = 0; i < state->outTextureIds.size(); ++i) {
        JS_SetPropertyUint32(
            ctx, outTextures, i,
            syncTextureWrapper(ctx, host, state->outTextureIds[i]));
    }

    setProperty(ctx, wrapper, ATLAS_RENDER_TARGET_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(renderTargetId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(
        ctx, wrapper, "type",
        JS_NewInt32(ctx, toScriptRenderTargetType(state->renderTarget->type)));
    setProperty(ctx, wrapper, "resolution",
                JS_NewInt32(ctx, state->renderTarget->getWidth()));
    setProperty(ctx, wrapper, "outTextures", outTextures);
    if (state->depthTextureId != 0) {
        setProperty(ctx, wrapper, "depthTexture",
                    syncTextureWrapper(ctx, host, state->depthTextureId));
    } else {
        setProperty(ctx, wrapper, "depthTexture", JS_NULL);
    }
    return wrapper;
}

void syncPointLightDebugObject(Light &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.debugObject->setColor(light.color);
}

void syncSpotLightDebugObject(Spotlight &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.setColor(light.color);
    light.updateDebugObjectRotation();
}

void syncAreaLightDebugObject(AreaLight &light) {
    if (light.debugObject == nullptr) {
        return;
    }
    light.debugObject->setPosition(light.position);
    light.debugObject->lookAt(light.position + light.getNormal(), light.up);
    light.debugObject->material.albedo = light.color;
    light.debugObject->material.emissiveColor = light.color;
    light.debugObject->material.emissiveIntensity =
        std::clamp(light.intensity * 0.2f, 1.0f, 8.0f);
}

JSValue syncPointLightWrapper(JSContext *ctx, ScriptHost &host,
                              std::uint64_t lightId) {
    auto *state = findPointLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.pointLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_POINT_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    setProperty(ctx, wrapper, "distance",
                JS_NewFloat64(ctx, state->light->distance));
    return wrapper;
}

JSValue syncDirectionalLightWrapper(JSContext *ctx, ScriptHost &host,
                                    std::uint64_t lightId) {
    auto *state = findDirectionalLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.directionalLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_DIRECTIONAL_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "direction",
                makePosition3d(ctx, host, state->light->direction));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    return wrapper;
}

JSValue syncSpotLightWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t lightId) {
    auto *state = findSpotLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.spotLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    const double cutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(state->light->cutOff), -1.0, 1.0)));
    const double outerCutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(state->light->outerCutoff), -1.0, 1.0)));

    setProperty(ctx, wrapper, ATLAS_SPOT_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "direction",
                makePosition3d(ctx, host, state->light->direction));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "range", JS_NewFloat64(ctx, state->light->range));
    setProperty(ctx, wrapper, "cutOff", JS_NewFloat64(ctx, cutOff));
    setProperty(ctx, wrapper, "outerCutOff", JS_NewFloat64(ctx, outerCutOff));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    return wrapper;
}

JSValue syncAreaLightWrapper(JSContext *ctx, ScriptHost &host,
                             std::uint64_t lightId) {
    auto *state = findAreaLightState(host, lightId);
    if (state == nullptr || state->light == nullptr) {
        return JS_NULL;
    }
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(state->value)) {
        wrapper = JS_DupValue(ctx, state->value);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.areaLightPrototype);
        state->value = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_AREA_LIGHT_ID_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(lightId)));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, state->light->position));
    setProperty(ctx, wrapper, "right",
                makePosition3d(ctx, host, state->light->right));
    setProperty(ctx, wrapper, "up",
                makePosition3d(ctx, host, state->light->up));
    setProperty(ctx, wrapper, "size",
                makeSize2d(ctx, host, state->light->size));
    setProperty(ctx, wrapper, "color",
                makeColor(ctx, host, state->light->color));
    setProperty(ctx, wrapper, "shineColor",
                makeColor(ctx, host, state->light->shineColor));
    setProperty(ctx, wrapper, "intensity",
                JS_NewFloat64(ctx, state->light->intensity));
    setProperty(ctx, wrapper, "range", JS_NewFloat64(ctx, state->light->range));
    setProperty(ctx, wrapper, "angle", JS_NewFloat64(ctx, state->light->angle));
    setProperty(ctx, wrapper, "castsBothSides",
                JS_NewBool(ctx, state->light->castsBothSides));
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, state->light->rotation));
    return wrapper;
}

JSValue syncSceneWrapper(JSContext *ctx, ScriptHost &host, Scene &scene) {
    (void)scene;
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.sceneValue)) {
        wrapper = JS_DupValue(ctx, host.sceneValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.scenePrototype);
        host.sceneValue = JS_DupValue(ctx, wrapper);
    }

    std::string name;
    if (host.context != nullptr) {
        name = host.context->currentSceneName;
    }

    setProperty(ctx, wrapper, ATLAS_SCENE_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "name", JS_NewString(ctx, name.c_str()));
    return wrapper;
}

Texture createEmptyTexture(int width, int height, TextureType type,
                           Color borderColor) {
    opal::TextureFormat format = opal::TextureFormat::Rgba8;
    opal::TextureDataFormat dataFormat = opal::TextureDataFormat::Rgba;

    switch (type) {
    case TextureType::Depth:
    case TextureType::DepthCube:
        format = opal::TextureFormat::DepthComponent24;
        dataFormat = opal::TextureDataFormat::DepthComponent;
        break;
    case TextureType::HDR:
        format = opal::TextureFormat::Rgba16F;
        dataFormat = opal::TextureDataFormat::Rgba;
        break;
    case TextureType::AO:
    case TextureType::Opacity:
    case TextureType::SSAONoise:
    case TextureType::SSAO:
        format = opal::TextureFormat::Red8;
        dataFormat = opal::TextureDataFormat::Red;
        break;
    default:
        break;
    }

    return Texture::create(width, height, format, dataFormat, type, {},
                           borderColor);
}

Texture createSolidTexture(Color color, TextureType type, int width,
                           int height) {
    const int checkSize = std::max(width, height);
    Texture texture =
        Texture::createCheckerboard(width, height, checkSize, color, color);
    texture.type = type;
    return texture;
}

std::shared_ptr<Effect> parseEffectValue(JSContext *ctx, JSValueConst value) {
    std::string type;
    if (JS_IsString(value)) {
        const char *name = JS_ToCString(ctx, value);
        if (name == nullptr) {
            return nullptr;
        }
        type = name;
        JS_FreeCString(ctx, name);
    } else {
        if (!readStringProperty(ctx, value, "type", type)) {
            return nullptr;
        }
    }

    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "inversion" || normalizedType == "invert") {
        return Inversion::create();
    }
    if (normalizedType == "grayscale") {
        return Grayscale::create();
    }
    if (normalizedType == "sharpen") {
        return Sharpen::create();
    }
    if (normalizedType == "blur") {
        double magnitude = 16.0;
        if (!JS_IsString(value)) {
            readNumberProperty(ctx, value, "magnitude", magnitude);
        }
        return Blur::create(static_cast<float>(magnitude));
    }
    if (normalizedType == "edgedetection") {
        return EdgeDetection::create();
    }
    if (normalizedType == "colorcorrection") {
        ColorCorrectionParameters params;
        if (!JS_IsString(value)) {
            double exposure = params.exposure;
            double contrast = params.contrast;
            double saturation = params.saturation;
            double gamma = params.gamma;
            double temperature = params.temperature;
            double tint = params.tint;
            readNumberProperty(ctx, value, "exposure", exposure);
            readNumberProperty(ctx, value, "contrast", contrast);
            readNumberProperty(ctx, value, "saturation", saturation);
            readNumberProperty(ctx, value, "gamma", gamma);
            readNumberProperty(ctx, value, "temperature", temperature);
            readNumberProperty(ctx, value, "tint", tint);
            params.exposure = static_cast<float>(exposure);
            params.contrast = static_cast<float>(contrast);
            params.saturation = static_cast<float>(saturation);
            params.gamma = static_cast<float>(gamma);
            params.temperature = static_cast<float>(temperature);
            params.tint = static_cast<float>(tint);
        }
        return ColorCorrection::create(params);
    }
    if (normalizedType == "motionblur") {
        MotionBlurParameters params;
        if (!JS_IsString(value)) {
            std::int64_t size = params.size;
            JSValue sizeValue = JS_GetPropertyStr(ctx, value, "size");
            if (!JS_IsException(sizeValue)) {
                getInt64(ctx, sizeValue, size);
            }
            JS_FreeValue(ctx, sizeValue);
            double separation = params.separation;
            readNumberProperty(ctx, value, "separation", separation);
            params.size = static_cast<int>(size);
            params.separation = static_cast<float>(separation);
        }
        return MotionBlur::create(params);
    }
    if (normalizedType == "chromaticaberration") {
        ChromaticAberrationParameters params;
        if (!JS_IsString(value)) {
            double red = params.red;
            double green = params.green;
            double blue = params.blue;
            readNumberProperty(ctx, value, "red", red);
            readNumberProperty(ctx, value, "green", green);
            readNumberProperty(ctx, value, "blue", blue);
            params.red = static_cast<float>(red);
            params.green = static_cast<float>(green);
            params.blue = static_cast<float>(blue);
            JSValue direction = JS_GetPropertyStr(ctx, value, "direction");
            if (!JS_IsException(direction) && !JS_IsUndefined(direction)) {
                parsePosition2d(ctx, direction, params.direction);
            }
            JS_FreeValue(ctx, direction);
        }
        return ChromaticAberration::create(params);
    }
    if (normalizedType == "posterization") {
        PosterizationParameters params;
        if (!JS_IsString(value)) {
            double levels = params.levels;
            readNumberProperty(ctx, value, "levels", levels);
            params.levels = static_cast<float>(levels);
        }
        return Posterization::create(params);
    }
    if (normalizedType == "pixelation") {
        PixelationParameters params;
        if (!JS_IsString(value)) {
            std::int64_t pixelSize = params.pixelSize;
            JSValue pixelSizeValue = JS_GetPropertyStr(ctx, value, "pixelSize");
            if (!JS_IsException(pixelSizeValue)) {
                getInt64(ctx, pixelSizeValue, pixelSize);
            }
            JS_FreeValue(ctx, pixelSizeValue);
            params.pixelSize = static_cast<int>(pixelSize);
        }
        return Pixelation::create(params);
    }
    if (normalizedType == "dialation" || normalizedType == "dilation") {
        DilationParameters params;
        if (!JS_IsString(value)) {
            std::int64_t size = params.size;
            JSValue sizeValue = JS_GetPropertyStr(ctx, value, "size");
            if (!JS_IsException(sizeValue)) {
                getInt64(ctx, sizeValue, size);
            }
            JS_FreeValue(ctx, sizeValue);
            double separation = params.separation;
            readNumberProperty(ctx, value, "separation", separation);
            params.size = static_cast<int>(size);
            params.separation = static_cast<float>(separation);
        }
        return Dilation::create(params);
    }
    if (normalizedType == "filmgrain") {
        FilmGrainParameters params;
        if (!JS_IsString(value)) {
            double amount = params.amount;
            readNumberProperty(ctx, value, "amount", amount);
            params.amount = static_cast<float>(amount);
        }
        return FilmGrain::create(params);
    }

    return nullptr;
}

JSValue syncCameraWrapper(JSContext *ctx, ScriptHost &host, Camera &camera) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    JSValue wrapper = JS_UNDEFINED;
    if (!JS_IsUndefined(host.cameraValue)) {
        wrapper = JS_DupValue(ctx, host.cameraValue);
    } else {
        wrapper = newObjectFromPrototype(ctx, host.cameraPrototype);
        host.cameraValue = JS_DupValue(ctx, wrapper);
    }

    setProperty(ctx, wrapper, ATLAS_CAMERA_PROP, JS_NewBool(ctx, true));
    setProperty(ctx, wrapper, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host.generation)));
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, camera.position));
    setProperty(ctx, wrapper, "target",
                makePosition3d(ctx, host, camera.target));
    setProperty(ctx, wrapper, "fov", JS_NewFloat64(ctx, camera.fov));
    setProperty(ctx, wrapper, "nearClip", JS_NewFloat64(ctx, camera.nearClip));
    setProperty(ctx, wrapper, "farClip", JS_NewFloat64(ctx, camera.farClip));
    setProperty(ctx, wrapper, "orthographicSize",
                JS_NewFloat64(ctx, camera.orthographicSize));
    setProperty(ctx, wrapper, "movementSpeed",
                JS_NewFloat64(ctx, camera.movementSpeed));
    setProperty(ctx, wrapper, "mouseSensitivity",
                JS_NewFloat64(ctx, camera.mouseSensitivity));
    setProperty(ctx, wrapper, "controllerLookSensitivity",
                JS_NewFloat64(ctx, camera.controllerLookSensitivity));
    setProperty(ctx, wrapper, "lookSmoothness",
                JS_NewFloat64(ctx, camera.lookSmoothness));
    setProperty(ctx, wrapper, "useOrthographic",
                JS_NewBool(ctx, camera.useOrthographic));
    setProperty(ctx, wrapper, "focusDepth",
                JS_NewFloat64(ctx, camera.focusDepth));
    setProperty(ctx, wrapper, "focusRange",
                JS_NewFloat64(ctx, camera.focusRange));
    return wrapper;
}

bool applyCamera(JSContext *ctx, JSValueConst wrapper, Camera &camera) {
    Position3d position = camera.position;
    Point3d target = camera.target;
    double fov = camera.fov;
    double nearClip = camera.nearClip;
    double farClip = camera.farClip;
    double orthographicSize = camera.orthographicSize;
    double movementSpeed = camera.movementSpeed;
    double mouseSensitivity = camera.mouseSensitivity;
    double controllerLookSensitivity = camera.controllerLookSensitivity;
    double lookSmoothness = camera.lookSmoothness;
    bool useOrthographic = camera.useOrthographic;
    double focusDepth = camera.focusDepth;
    double focusRange = camera.focusRange;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "target");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, target);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "fov", fov);
    readNumberProperty(ctx, wrapper, "nearClip", nearClip);
    readNumberProperty(ctx, wrapper, "farClip", farClip);
    readNumberProperty(ctx, wrapper, "orthographicSize", orthographicSize);
    readNumberProperty(ctx, wrapper, "movementSpeed", movementSpeed);
    readNumberProperty(ctx, wrapper, "mouseSensitivity", mouseSensitivity);
    readNumberProperty(ctx, wrapper, "controllerLookSensitivity",
                       controllerLookSensitivity);
    readNumberProperty(ctx, wrapper, "lookSmoothness", lookSmoothness);
    readBoolProperty(ctx, wrapper, "useOrthographic", useOrthographic);
    readNumberProperty(ctx, wrapper, "focusDepth", focusDepth);
    readNumberProperty(ctx, wrapper, "focusRange", focusRange);

    camera.setPosition(position);
    if (target.x != position.x || target.y != position.y ||
        target.z != position.z) {
        camera.lookAt(target);
    } else {
        camera.target = target;
    }
    camera.fov = static_cast<float>(fov);
    camera.nearClip = static_cast<float>(nearClip);
    camera.farClip = static_cast<float>(farClip);
    camera.orthographicSize = static_cast<float>(orthographicSize);
    camera.movementSpeed = static_cast<float>(movementSpeed);
    camera.mouseSensitivity = static_cast<float>(mouseSensitivity);
    camera.controllerLookSensitivity =
        static_cast<float>(controllerLookSensitivity);
    camera.lookSmoothness = static_cast<float>(lookSmoothness);
    camera.useOrthographic = useOrthographic;
    camera.focusDepth = static_cast<float>(focusDepth);
    camera.focusRange = static_cast<float>(focusRange);

    return true;
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

    if (auto *core = dynamic_cast<CoreObject *>(&object);
        core != nullptr && core->vertices.empty()) {
        return;
    }

    object.initialize();
    host.context->window->addObject(&object);
    state.attachedToWindow = true;
}

void syncObjectTextureStates(ScriptHost &host, CoreObject &object) {
    auto &state = host.objectStates[object.getId()];
    std::vector<std::uint64_t> currentTextureIds = state.textureIds;
    state.textureIds.clear();

    auto assignTexture = [&](std::uint64_t &slot, const Texture &texture) {
        if (texture.id == 0 && texture.texture == nullptr) {
            slot = 0;
            return;
        }
        if (slot == 0) {
            slot = registerTextureState(host, texture);
            return;
        }
        auto *textureState = findTextureState(host, slot);
        if (textureState != nullptr && textureState->texture) {
            *textureState->texture = texture;
            return;
        }
        slot = registerTextureState(host, texture);
    };

    for (std::size_t i = 0; i < object.textures.size(); ++i) {
        std::uint64_t slot = 0;
        if (i < currentTextureIds.size()) {
            slot = currentTextureIds[i];
        }
        assignTexture(slot, object.textures[i]);
        if (slot != 0) {
            state.textureIds.push_back(slot);
        }
    }
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host, GameObject &object);

JSValue syncInstanceWrapper(JSContext *ctx, ScriptHost &host,
                            CoreObject &object, std::uint32_t index) {
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
    setProperty(ctx, wrapper, "position",
                makePosition3d(ctx, host, instance.position));
    setProperty(ctx, wrapper, "rotation",
                makeRotation3d(ctx, host, instance.rotation));
    setProperty(ctx, wrapper, "scale",
                makePosition3d(ctx, host, instance.scale));
    return wrapper;
}

JSValue makeMaterial(JSContext *ctx, ScriptHost &host,
                     const Material &material) {
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

JSValue makeCoreVertex(JSContext *ctx, ScriptHost &host,
                       const CoreVertex &vertex) {
    JSValue result = newObjectFromPrototype(ctx, host.coreVertexPrototype);
    setProperty(ctx, result, "position",
                makePosition3d(ctx, host, vertex.position));
    setProperty(ctx, result, "color", makeColor(ctx, host, vertex.color));
    setProperty(ctx, result, "textureCoord",
                makePosition2d(ctx, host,
                               Position2d(vertex.textureCoordinate[0],
                                          vertex.textureCoordinate[1])));
    setProperty(ctx, result, "normal",
                makePosition3d(ctx, host, vertex.normal));
    setProperty(ctx, result, "tangent",
                makePosition3d(ctx, host, vertex.tangent));
    setProperty(ctx, result, "bitangent",
                makePosition3d(ctx, host, vertex.bitangent));
    return result;
}

JSValue syncObjectWrapper(JSContext *ctx, ScriptHost &host,
                          GameObject &object) {
    if (!ensureBuiltins(ctx, host)) {
        return JS_EXCEPTION;
    }

    const int objectId = object.getId();
    JSValue wrapper = JS_UNDEFINED;
    auto cacheIt = host.objectCache.find(objectId);
    if (cacheIt != host.objectCache.end()) {
        wrapper = JS_DupValue(ctx, cacheIt->second);
    } else {
        JSValueConst prototype = host.gameObjectPrototype;
        if (dynamic_cast<CoreObject *>(&object) != nullptr) {
            prototype = host.coreObjectPrototype;
        } else if (dynamic_cast<ParticleEmitter *>(&object) != nullptr) {
            prototype = host.particleEmitterPrototype;
        } else if (dynamic_cast<Model *>(&object) != nullptr) {
            prototype = host.modelPrototype;
        }
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
    setProperty(
        ctx, wrapper, ATLAS_IS_CORE_OBJECT_PROP,
        JS_NewBool(ctx, dynamic_cast<CoreObject *>(&object) != nullptr));
    setProperty(ctx, wrapper, "id", JS_NewInt32(ctx, objectId));
    setProperty(ctx, wrapper, "components",
                buildComponentsArray(ctx, host, objectId));
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

    if (auto *emitter = dynamic_cast<ParticleEmitter *>(&object);
        emitter != nullptr) {
        setProperty(ctx, wrapper, ATLAS_PARTICLE_EMITTER_ID_PROP,
                    JS_NewInt32(ctx, objectId));
        setProperty(ctx, wrapper, "settings",
                    makeParticleSettings(ctx, emitter->settings));
        setProperty(ctx, wrapper, "position",
                    makePosition3d(ctx, host, emitter->getPosition()));
    }

    if (auto *core = dynamic_cast<CoreObject *>(&object); core != nullptr) {
        syncObjectTextureStates(host, *core);

        JSValue vertices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->vertices.size(); ++i) {
            JS_SetPropertyUint32(ctx, vertices, i,
                                 makeCoreVertex(ctx, host, core->vertices[i]));
        }

        JSValue indices = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->indices.size(); ++i) {
            JS_SetPropertyUint32(
                ctx, indices, i,
                JS_NewInt32(ctx, static_cast<int>(core->indices[i])));
        }

        JSValue instances = JS_NewArray(ctx);
        for (std::uint32_t i = 0; i < core->instances.size(); ++i) {
            JS_SetPropertyUint32(ctx, instances, i,
                                 syncInstanceWrapper(ctx, host, *core, i));
        }

        JSValue textures = JS_NewArray(ctx);
        auto stateIt = host.objectStates.find(objectId);
        if (stateIt != host.objectStates.end()) {
            for (std::uint32_t i = 0; i < stateIt->second.textureIds.size();
                 ++i) {
                JS_SetPropertyUint32(
                    ctx, textures, i,
                    syncTextureWrapper(ctx, host,
                                       stateIt->second.textureIds[i]));
            }
        }

        setProperty(ctx, wrapper, "vertices", vertices);
        setProperty(ctx, wrapper, "indices", indices);
        setProperty(ctx, wrapper, "textures", textures);
        setProperty(ctx, wrapper, "material",
                    makeMaterial(ctx, host, core->material));
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

bool applyPointLight(JSContext *ctx, JSValueConst wrapper, Light &light) {
    Position3d position = light.position;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;
    double distance = light.distance;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);
    readNumberProperty(ctx, wrapper, "distance", distance);

    light.position = position;
    light.setColor(color);
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    light.distance = static_cast<float>(distance);
    syncPointLightDebugObject(light);
    return true;
}

bool applyDirectionalLight(JSContext *ctx, JSValueConst wrapper,
                           DirectionalLight &light) {
    Magnitude3d direction = light.direction;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "direction");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, direction);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);

    light.direction = direction.normalized();
    light.color = color;
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    return true;
}

bool applySpotLight(JSContext *ctx, JSValueConst wrapper, Spotlight &light) {
    Position3d position = light.position;
    Magnitude3d direction = light.direction;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double range = light.range;
    double cutOff = glm::degrees(
        std::acos(std::clamp(static_cast<double>(light.cutOff), -1.0, 1.0)));
    double outerCutOff = glm::degrees(std::acos(
        std::clamp(static_cast<double>(light.outerCutoff), -1.0, 1.0)));
    double intensity = light.intensity;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "direction");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, direction);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "range", range);
    readNumberProperty(ctx, wrapper, "cutOff", cutOff);
    readNumberProperty(ctx, wrapper, "outerCutOff", outerCutOff);
    readNumberProperty(ctx, wrapper, "intensity", intensity);

    light.position = position;
    light.direction = direction.normalized();
    light.setColor(color);
    light.shineColor = shineColor;
    light.range = static_cast<float>(range);
    light.cutOff =
        glm::cos(glm::radians(static_cast<double>(static_cast<float>(cutOff))));
    light.outerCutoff = glm::cos(
        glm::radians(static_cast<double>(static_cast<float>(outerCutOff))));
    light.intensity = static_cast<float>(intensity);
    syncSpotLightDebugObject(light);
    return true;
}

bool applyAreaLight(JSContext *ctx, JSValueConst wrapper, AreaLight &light) {
    Position3d position = light.position;
    Magnitude3d right = light.right;
    Magnitude3d up = light.up;
    Size2d size = light.size;
    Color color = light.color;
    Color shineColor = light.shineColor;
    double intensity = light.intensity;
    double range = light.range;
    double angle = light.angle;
    bool castsBothSides = light.castsBothSides;
    Rotation3d rotation = light.rotation;

    JSValue value = JS_GetPropertyStr(ctx, wrapper, "position");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, position);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "right");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, right);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "up");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parsePosition3d(ctx, value, up);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "size");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseSize2d(ctx, value, size);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "color");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, color);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "shineColor");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseColor(ctx, value, shineColor);
    }
    JS_FreeValue(ctx, value);

    value = JS_GetPropertyStr(ctx, wrapper, "rotation");
    if (!JS_IsException(value) && !JS_IsUndefined(value)) {
        parseRotation3d(ctx, value, rotation);
    }
    JS_FreeValue(ctx, value);

    readNumberProperty(ctx, wrapper, "intensity", intensity);
    readNumberProperty(ctx, wrapper, "range", range);
    readNumberProperty(ctx, wrapper, "angle", angle);
    readBoolProperty(ctx, wrapper, "castsBothSides", castsBothSides);

    light.position = position;
    light.right = right.normalized();
    light.up = up.normalized();
    light.size = size;
    light.setColor(color);
    light.shineColor = shineColor;
    light.intensity = static_cast<float>(intensity);
    light.range = static_cast<float>(range);
    light.angle = static_cast<float>(angle);
    light.castsBothSides = castsBothSides;
    light.rotation = rotation;
    syncAreaLightDebugObject(light);
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

Model *resolveModelArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *model = dynamic_cast<Model *>(object);
    if (model == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a Model");
        return nullptr;
    }
    return model;
}

ParticleEmitter *resolveParticleEmitterArg(JSContext *ctx, ScriptHost &host,
                                           JSValueConst value) {
    GameObject *object = resolveObjectArg(ctx, host, value);
    if (object == nullptr) {
        return nullptr;
    }
    auto *emitter = dynamic_cast<ParticleEmitter *>(object);
    if (emitter == nullptr) {
        JS_ThrowTypeError(ctx, "Atlas object is not a ParticleEmitter");
        return nullptr;
    }
    return emitter;
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

Camera *resolveCameraArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isCamera = false;
    if (!readBoolProperty(ctx, value, ATLAS_CAMERA_PROP, isCamera) ||
        !isCamera) {
        JS_ThrowTypeError(ctx, "Expected Atlas camera handle");
        return nullptr;
    }

    Camera *camera = getSceneCamera(host);
    if (camera == nullptr) {
        JS_ThrowReferenceError(ctx, "Atlas camera is unavailable");
    }
    return camera;
}

Scene *resolveSceneArg(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    bool isScene = false;
    if (!readBoolProperty(ctx, value, ATLAS_SCENE_PROP, isScene) || !isScene) {
        JS_ThrowTypeError(ctx, "Expected Atlas scene handle");
        return nullptr;
    }

    Scene *scene = getScene(host);
    if (scene == nullptr) {
        JS_ThrowReferenceError(ctx, "Atlas scene is unavailable");
    }
    return scene;
}

ScriptTextureState *resolveTexture(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t textureId = 0;
    if (!readIntProperty(ctx, value, ATLAS_TEXTURE_ID_PROP, textureId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas texture handle");
        return nullptr;
    }

    auto *state = findTextureState(host, static_cast<std::uint64_t>(textureId));
    if (state == nullptr || !state->texture) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas texture id");
        return nullptr;
    }
    return state;
}

ScriptCubemapState *resolveCubemap(JSContext *ctx, ScriptHost &host,
                                   JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t cubemapId = 0;
    if (!readIntProperty(ctx, value, ATLAS_CUBEMAP_ID_PROP, cubemapId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas cubemap handle");
        return nullptr;
    }

    auto *state = findCubemapState(host, static_cast<std::uint64_t>(cubemapId));
    if (state == nullptr || !state->cubemap) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas cubemap id");
        return nullptr;
    }
    return state;
}

ScriptSkyboxState *resolveSkybox(JSContext *ctx, ScriptHost &host,
                                 JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t skyboxId = 0;
    if (!readIntProperty(ctx, value, ATLAS_SKYBOX_ID_PROP, skyboxId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas skybox handle");
        return nullptr;
    }

    auto *state = findSkyboxState(host, static_cast<std::uint64_t>(skyboxId));
    if (state == nullptr || !state->skybox) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas skybox id");
        return nullptr;
    }
    return state;
}

ScriptRenderTargetState *resolveRenderTarget(JSContext *ctx, ScriptHost &host,
                                             JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t renderTargetId = 0;
    if (!readIntProperty(ctx, value, ATLAS_RENDER_TARGET_ID_PROP,
                         renderTargetId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas render target handle");
        return nullptr;
    }

    auto *state =
        findRenderTargetState(host, static_cast<std::uint64_t>(renderTargetId));
    if (state == nullptr || !state->renderTarget) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas render target id");
        return nullptr;
    }
    return state;
}

ScriptPointLightState *resolvePointLight(JSContext *ctx, ScriptHost &host,
                                         JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_POINT_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas point light handle");
        return nullptr;
    }

    auto *state =
        findPointLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas point light id");
        return nullptr;
    }
    return state;
}

ScriptDirectionalLightState *
resolveDirectionalLight(JSContext *ctx, ScriptHost &host, JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_DIRECTIONAL_LIGHT_ID_PROP,
                         lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas directional light handle");
        return nullptr;
    }

    auto *state =
        findDirectionalLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas directional light id");
        return nullptr;
    }
    return state;
}

ScriptSpotLightState *resolveSpotLight(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_SPOT_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas spot light handle");
        return nullptr;
    }

    auto *state = findSpotLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas spot light id");
        return nullptr;
    }
    return state;
}

ScriptAreaLightState *resolveAreaLight(JSContext *ctx, ScriptHost &host,
                                       JSValueConst value) {
    if (!ensureCurrentGeneration(ctx, host, value)) {
        return nullptr;
    }

    std::int64_t lightId = 0;
    if (!readIntProperty(ctx, value, ATLAS_AREA_LIGHT_ID_PROP, lightId)) {
        JS_ThrowTypeError(ctx, "Expected Atlas area light handle");
        return nullptr;
    }

    auto *state = findAreaLightState(host, static_cast<std::uint64_t>(lightId));
    if (state == nullptr || state->light == nullptr) {
        JS_ThrowReferenceError(ctx, "Unknown Atlas area light id");
        return nullptr;
    }
    return state;
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

JSValue jsGetCamera(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Camera *camera = getSceneCamera(*host);
    if (camera == nullptr) {
        return JS_NULL;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsUpdateCamera(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected camera");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    if (!applyCamera(ctx, argv[0], *camera)) {
        return JS_EXCEPTION;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsSetPositionKeepingOrientation(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected camera and position");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    camera->setPositionKeepingOrientation(position);
    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsLookAtCamera(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected camera and target");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Point3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    if (target.x != camera->position.x || target.y != camera->position.y ||
        target.z != camera->position.z) {
        camera->lookAt(target);
    } else {
        camera->target = target;
    }

    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsMoveCameraTo(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx, "Expected camera, position, and speed");
    }

    Camera *camera = resolveCameraArg(ctx, *host, argv[0]);
    if (camera == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d targetPosition;
    if (!parsePosition3d(ctx, argv[1], targetPosition)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    double speed = 0.0;
    if (!getDouble(ctx, argv[2], speed)) {
        return JS_ThrowTypeError(ctx, "Expected speed");
    }

    const double dx = targetPosition.x - camera->position.x;
    const double dy = targetPosition.y - camera->position.y;
    const double dz = targetPosition.z - camera->position.z;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance == 0.0) {
        return syncCameraWrapper(ctx, *host, *camera);
    }

    if (speed <= 0.0 || distance <= speed) {
        camera->setPositionKeepingOrientation(targetPosition);
        return syncCameraWrapper(ctx, *host, *camera);
    }

    const double factor = speed / distance;
    camera->setPositionKeepingOrientation(Position3d(
        camera->position.x + dx * factor, camera->position.y + dy * factor,
        camera->position.z + dz * factor));
    return syncCameraWrapper(ctx, *host, *camera);
}

JSValue jsGetCameraDirection(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Camera *camera = nullptr;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        camera = resolveCameraArg(ctx, *host, argv[0]);
        if (camera == nullptr) {
            return JS_EXCEPTION;
        }
    } else {
        camera = getSceneCamera(*host);
    }

    if (camera == nullptr) {
        return JS_NULL;
    }

    return makePosition3d(ctx, *host, camera->getFrontVector());
}

JSValue jsGetScene(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr) {
        return JS_NULL;
    }

    Scene *scene = getScene(*host);
    if (scene == nullptr) {
        return JS_NULL;
    }

    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneAmbientIntensity(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and intensity");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    double intensity = 0.0;
    if (!getDouble(ctx, argv[1], intensity)) {
        return JS_ThrowTypeError(ctx, "Expected intensity");
    }

    scene->setAmbientIntensity(static_cast<float>(intensity));
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneAutomaticAmbient(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and enabled flag");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setAutomaticAmbient(JS_ToBool(ctx, argv[1]) == 1);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsSetSceneSkybox(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and skybox");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    auto *skyboxState = resolveSkybox(ctx, *host, argv[1]);
    if (skyboxState == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setSkybox(skyboxState->skybox);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsUseAtmosphereSkybox(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected scene and enabled flag");
    }

    Scene *scene = resolveSceneArg(ctx, *host, argv[0]);
    if (scene == nullptr) {
        return JS_EXCEPTION;
    }

    scene->setUseAtmosphereSkybox(JS_ToBool(ctx, argv[1]) == 1);
    return syncSceneWrapper(ctx, *host, *scene);
}

JSValue jsCreateTextureFromResource(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected resource and texture type");
    }

    Resource resource;
    if (JS_IsString(argv[0])) {
        const char *name = JS_ToCString(ctx, argv[0]);
        if (name == nullptr) {
            return JS_ThrowTypeError(ctx, "Expected resource");
        }
        resource = Workspace::get().getResource(name);
        JS_FreeCString(ctx, name);
    } else if (!parseResource(ctx, argv[0], resource)) {
        return JS_ThrowTypeError(ctx, "Expected Resource or resource name");
    }

    if (resource.name.empty() && resource.path.empty()) {
        return JS_ThrowReferenceError(ctx, "Unknown resource");
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        return JS_ThrowTypeError(ctx, "Expected texture type");
    }

    const std::uint64_t textureId = registerTextureState(
        *host, Texture::fromResource(
                   resource, toNativeTextureType(static_cast<int>(type))));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateEmptyTexture(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 3) {
        return JS_ThrowTypeError(ctx,
                                 "Expected width, height, and texture type");
    }

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t type = 0;
    if (!getInt64(ctx, argv[0], width) || !getInt64(ctx, argv[1], height) ||
        !getInt64(ctx, argv[2], type)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected width, height, and texture type");
    }

    Color borderColor(0.0f, 0.0f, 0.0f, 0.0f);
    if (argc > 3 && !JS_IsUndefined(argv[3])) {
        if (!parseColor(ctx, argv[3], borderColor)) {
            return JS_ThrowTypeError(ctx, "Expected border color");
        }
    }

    const std::uint64_t textureId = registerTextureState(
        *host, createEmptyTexture(
                   static_cast<int>(width), static_cast<int>(height),
                   toNativeTextureType(static_cast<int>(type)), borderColor));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateColorTexture(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 4) {
        return JS_ThrowTypeError(
            ctx, "Expected color, texture type, width, and height");
    }

    Color color;
    std::int64_t type = 0;
    std::int64_t width = 0;
    std::int64_t height = 0;
    if (!parseColor(ctx, argv[0], color) || !getInt64(ctx, argv[1], type) ||
        !getInt64(ctx, argv[2], width) || !getInt64(ctx, argv[3], height)) {
        return JS_ThrowTypeError(
            ctx, "Expected color, texture type, width, and height");
    }

    const std::uint64_t textureId = registerTextureState(
        *host,
        createSolidTexture(color, toNativeTextureType(static_cast<int>(type)),
                           static_cast<int>(width), static_cast<int>(height)));
    return syncTextureWrapper(ctx, *host, textureId);
}

JSValue jsCreateCheckerboardTexture(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 6) {
        return JS_ThrowTypeError(
            ctx, "Expected texture, width, height, check size, and two colors");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t checkSize = 0;
    Color color1;
    Color color2;
    if (!getInt64(ctx, argv[1], width) || !getInt64(ctx, argv[2], height) ||
        !getInt64(ctx, argv[3], checkSize) ||
        !parseColor(ctx, argv[4], color1) ||
        !parseColor(ctx, argv[5], color2)) {
        return JS_ThrowTypeError(
            ctx, "Expected texture, width, height, check size, and two colors");
    }

    *state->texture = Texture::createCheckerboard(
        static_cast<int>(width), static_cast<int>(height),
        static_cast<int>(checkSize), color1, color2);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsCreateDoubleCheckerboardTexture(JSContext *ctx, JSValueConst,
                                          int argc, JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 8) {
        return JS_ThrowTypeError(ctx, "Expected texture, width, height, two "
                                      "check sizes, and three colors");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);

    std::int64_t width = 0;
    std::int64_t height = 0;
    std::int64_t checkSizeBig = 0;
    std::int64_t checkSizeSmall = 0;
    Color color1;
    Color color2;
    Color color3;
    if (!getInt64(ctx, argv[1], width) || !getInt64(ctx, argv[2], height) ||
        !getInt64(ctx, argv[3], checkSizeBig) ||
        !getInt64(ctx, argv[4], checkSizeSmall) ||
        !parseColor(ctx, argv[5], color1) ||
        !parseColor(ctx, argv[6], color2) ||
        !parseColor(ctx, argv[7], color3)) {
        return JS_ThrowTypeError(ctx, "Expected texture, width, height, two "
                                      "check sizes, and three colors");
    }

    *state->texture = Texture::createDoubleCheckerboard(
        static_cast<int>(width), static_cast<int>(height),
        static_cast<int>(checkSizeBig), static_cast<int>(checkSizeSmall),
        color1, color2, color3);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsDisplayTexture(JSContext *ctx, JSValueConst, int argc,
                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected texture");
    }

    auto *state = resolveTexture(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->texture->display(*host->context->window);
    std::int64_t textureId = 0;
    readIntProperty(ctx, argv[0], ATLAS_TEXTURE_ID_PROP, textureId);
    return syncTextureWrapper(ctx, *host,
                              static_cast<std::uint64_t>(textureId));
}

JSValue jsCreateCubemap(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected resource array");
    }

    std::vector<Resource> resources;
    if (!parseResourceVector(ctx, argv[0], resources) ||
        resources.size() != 6) {
        return JS_ThrowTypeError(ctx, "Expected six cubemap resources");
    }

    ResourceGroup group;
    group.groupName = "ScriptCubemap";
    group.resources = resources;
    const std::uint64_t cubemapId =
        registerCubemapState(*host, Cubemap::fromResourceGroup(group));
    return syncCubemapWrapper(ctx, *host, cubemapId);
}

JSValue jsCreateCubemapFromGroup(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    return jsCreateCubemap(ctx, JS_UNDEFINED, argc, argv);
}

JSValue jsGetCubemapAverageColor(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected cubemap");
    }

    auto *state = resolveCubemap(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    return makeColor(ctx, *host, state->cubemap->averageColor);
}

JSValue jsUpdateCubemapWithColors(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected cubemap and six colors");
    }

    auto *state = resolveCubemap(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::array<Color, 6> colors;
    if (!parseColorArray(ctx, argv[1], colors)) {
        return JS_ThrowTypeError(ctx, "Expected an array of six colors");
    }

    state->cubemap->updateWithColors(colors);
    std::int64_t cubemapId = 0;
    readIntProperty(ctx, argv[0], ATLAS_CUBEMAP_ID_PROP, cubemapId);
    return syncCubemapWrapper(ctx, *host,
                              static_cast<std::uint64_t>(cubemapId));
}

JSValue jsCreateSkybox(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected cubemap");
    }

    auto *cubemapState = resolveCubemap(ctx, *host, argv[0]);
    if (cubemapState == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t cubemapId = 0;
    readIntProperty(ctx, argv[0], ATLAS_CUBEMAP_ID_PROP, cubemapId);

    const std::uint64_t skyboxId = host->nextSkyboxId++;
    host->skyboxes[skyboxId] = {
        .skybox =
            Skybox::create(*cubemapState->cubemap, *host->context->window),
        .value = JS_UNDEFINED,
        .cubemapId = static_cast<std::uint64_t>(cubemapId)};
    return syncSkyboxWrapper(ctx, *host, skyboxId);
}

JSValue jsCreateRenderTarget(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    std::int64_t type = 0;
    std::int64_t resolution = 1024;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !getInt64(ctx, argv[0], type)) {
        return JS_ThrowTypeError(ctx, "Expected render target type");
    }
    if (argc > 1 && !JS_IsUndefined(argv[1]) &&
        !getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    const std::uint64_t renderTargetId = host->nextRenderTargetId++;
    host->renderTargets[renderTargetId] = {
        .renderTarget = std::make_shared<RenderTarget>(
            *host->context->window,
            toNativeRenderTargetType(static_cast<int>(type)),
            static_cast<int>(resolution)),
        .value = JS_UNDEFINED,
        .attached = false,
        .outTextureIds = {},
        .depthTextureId = 0};
    return syncRenderTargetWrapper(ctx, *host, renderTargetId);
}

JSValue jsAddRenderTargetToPassQueue(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected render target and pass type");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t passType = 0;
    if (!getInt64(ctx, argv[1], passType)) {
        return JS_ThrowTypeError(ctx, "Expected pass type");
    }

    switch (passType) {
    case 0:
        host->context->window->useDeferredRendering();
        break;
    case 1:
        host->context->window->usesDeferred = false;
        host->context->window->usePathTracing = false;
        break;
    case 2:
        host->context->window->enablePathTracing();
        break;
    default:
        return JS_ThrowRangeError(ctx, "Unknown render pass type");
    }

    if (!state->attached) {
        host->context->window->addRenderTarget(state->renderTarget.get());
        state->attached = true;
    }

    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsAddRenderTargetEffect(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected render target and effect");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    auto effect = parseEffectValue(ctx, argv[1]);
    if (effect == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected Atlas effect");
    }

    state->renderTarget->addEffect(effect);

    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsDisplayRenderTarget(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected render target");
    }

    auto *state = resolveRenderTarget(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    state->renderTarget->display(*host->context->window);
    std::int64_t renderTargetId = 0;
    readIntProperty(ctx, argv[0], ATLAS_RENDER_TARGET_ID_PROP, renderTargetId);
    return syncRenderTargetWrapper(ctx, *host,
                                   static_cast<std::uint64_t>(renderTargetId));
}

JSValue jsCreatePointLight(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<Light>();
    applyPointLight(ctx, argv[0], *light);
    scene->addLight(light.get());

    const std::uint64_t lightId = host->nextPointLightId++;
    host->pointLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->pointLights.push_back(std::move(light));
    return syncPointLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdatePointLight(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected point light");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyPointLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCreatePointLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected point light");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyPointLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncPointLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCastPointLightShadows(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected point light and resolution");
    }

    auto *state = resolvePointLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyPointLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_POINT_LIGHT_ID_PROP, lightId);
    return syncPointLightWrapper(ctx, *host,
                                 static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateDirectionalLight(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<DirectionalLight>();
    applyDirectionalLight(ctx, argv[0], *light);
    scene->addDirectionalLight(light.get());

    const std::uint64_t lightId = host->nextDirectionalLightId++;
    host->directionalLights[lightId] = {.light = light.get(),
                                        .value = JS_UNDEFINED};
    host->context->directionalLights.push_back(std::move(light));
    return syncDirectionalLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateDirectionalLight(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected directional light");
    }

    auto *state = resolveDirectionalLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyDirectionalLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_DIRECTIONAL_LIGHT_ID_PROP, lightId);
    return syncDirectionalLightWrapper(ctx, *host,
                                       static_cast<std::uint64_t>(lightId));
}

JSValue jsCastDirectionalLightShadows(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected directional light and resolution");
    }

    auto *state = resolveDirectionalLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyDirectionalLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_DIRECTIONAL_LIGHT_ID_PROP, lightId);
    return syncDirectionalLightWrapper(ctx, *host,
                                       static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<Spotlight>();
    applySpotLight(ctx, argv[0], *light);
    scene->addSpotlight(light.get());

    const std::uint64_t lightId = host->nextSpotLightId++;
    host->spotLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->spotlights.push_back(std::move(light));
    return syncSpotLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spot light");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateSpotLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected spot light");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncSpotLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsLookAtSpotLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected spot light and target");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applySpotLight(ctx, argv[0], *state->light);
    Position3d target;
    if (!parsePosition3d(ctx, argv[1], target)) {
        return JS_ThrowTypeError(ctx, "Expected target Position3d");
    }

    state->light->lookAt(target);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCastSpotLightShadows(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected spot light and resolution");
    }

    auto *state = resolveSpotLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applySpotLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_SPOT_LIGHT_ID_PROP, lightId);
    return syncSpotLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    Scene *scene = host != nullptr ? getScene(*host) : nullptr;
    if (host == nullptr || host->context == nullptr || scene == nullptr ||
        argc < 1) {
        return JS_ThrowInternalError(ctx, "Atlas scripting host unavailable");
    }

    auto light = std::make_unique<AreaLight>();
    applyAreaLight(ctx, argv[0], *light);
    scene->addAreaLight(light.get());

    const std::uint64_t lightId = host->nextAreaLightId++;
    host->areaLights[lightId] = {.light = light.get(), .value = JS_UNDEFINED};
    host->context->areaLights.push_back(std::move(light));
    return syncAreaLightWrapper(ctx, *host, lightId);
}

JSValue jsUpdateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsGetAreaLightNormal(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    return makePosition3d(ctx, *host, state->light->getNormal());
}

JSValue jsSetAreaLightRotation(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and rotation");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    state->light->setRotation(rotation);
    syncAreaLightDebugObject(*state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsRotateAreaLight(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and rotation");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    state->light->rotate(rotation);
    syncAreaLightDebugObject(*state->light);
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCreateAreaLightDebugObject(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected area light");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    applyAreaLight(ctx, argv[0], *state->light);
    if (state->light->debugObject == nullptr) {
        state->light->createDebugObject();
        state->light->addDebugObject(*host->context->window);
    } else {
        syncAreaLightDebugObject(*state->light);
    }

    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
}

JSValue jsCastAreaLightShadows(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected area light and resolution");
    }

    auto *state = resolveAreaLight(ctx, *host, argv[0]);
    if (state == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t resolution = 0;
    if (!getInt64(ctx, argv[1], resolution)) {
        return JS_ThrowTypeError(ctx, "Expected resolution");
    }

    applyAreaLight(ctx, argv[0], *state->light);
    state->light->castShadows(*host->context->window,
                              static_cast<int>(resolution));
    std::int64_t lightId = 0;
    readIntProperty(ctx, argv[0], ATLAS_AREA_LIGHT_ID_PROP, lightId);
    return syncAreaLightWrapper(ctx, *host,
                                static_cast<std::uint64_t>(lightId));
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
            resource.type !=
                toNativeResourceType(static_cast<int>(expectedType))) {
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
        return JS_ThrowTypeError(ctx,
                                 "Expected audio player id and delta time");
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

JSValue jsGetInputConstants(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    JSValue result = JS_NewObject(ctx);
    JSValue keyObject = JS_NewObject(ctx);
    for (const auto &[name, key] : ATLAS_KEY_ENTRIES) {
        setProperty(ctx, keyObject, name,
                    JS_NewInt32(ctx, static_cast<int>(key)));
    }

    JSValue mouseButtonObject = JS_NewObject(ctx);
    for (const auto &[name, button] : ATLAS_MOUSE_BUTTON_ENTRIES) {
        setProperty(ctx, mouseButtonObject, name,
                    JS_NewInt32(ctx, static_cast<int>(button)));
    }

    JSValue triggerTypeObject = JS_NewObject(ctx);
    setProperty(ctx, triggerTypeObject, "MouseButton",
                JS_NewInt32(ctx, static_cast<int>(TriggerType::MouseButton)));
    setProperty(ctx, triggerTypeObject, "Key",
                JS_NewInt32(ctx, static_cast<int>(TriggerType::Key)));
    setProperty(
        ctx, triggerTypeObject, "ControllerButton",
        JS_NewInt32(ctx, static_cast<int>(TriggerType::ControllerButton)));

    JSValue axisTriggerTypeObject = JS_NewObject(ctx);
    setProperty(ctx, axisTriggerTypeObject, "MouseAxis",
                JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::MouseAxis)));
    setProperty(ctx, axisTriggerTypeObject, "KeyCustom",
                JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::KeyCustom)));
    setProperty(
        ctx, axisTriggerTypeObject, "ControllerAxis",
        JS_NewInt32(ctx, static_cast<int>(AxisTriggerType::ControllerAxis)));

    setProperty(ctx, result, "Key", keyObject);
    setProperty(ctx, result, "MouseButton", mouseButtonObject);
    setProperty(ctx, result, "TriggerType", triggerTypeObject);
    setProperty(ctx, result, "AxisTriggerType", axisTriggerTypeObject);
    return result;
}

JSValue jsRegisterInputAction(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected input action");
    }

    InputAction action;
    if (!parseInputAction(ctx, argv[0], action)) {
        return JS_ThrowTypeError(ctx, "Expected InputAction");
    }

    auto existing = host->context->window->getInputAction(action.name);
    if (existing != nullptr) {
        *existing = action;
    } else {
        host->context->window->addInputAction(
            std::make_shared<InputAction>(action));
    }

    return JS_DupValue(ctx, argv[0]);
}

JSValue jsResetInputActions(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->resetInputActions();
    }
    return JS_UNDEFINED;
}

JSValue jsIsKeyActive(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t key = 0;
    if (!getInt64(ctx, argv[0], key)) {
        return JS_ThrowTypeError(ctx, "Expected key");
    }
    return JS_NewBool(
        ctx, host->context->window->isKeyActive(static_cast<Key>(key)));
}

JSValue jsIsKeyPressed(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t key = 0;
    if (!getInt64(ctx, argv[0], key)) {
        return JS_ThrowTypeError(ctx, "Expected key");
    }
    return JS_NewBool(
        ctx, host->context->window->isKeyPressed(static_cast<Key>(key)));
}

JSValue jsIsMouseButtonActive(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t button = 0;
    if (!getInt64(ctx, argv[0], button)) {
        return JS_ThrowTypeError(ctx, "Expected mouse button");
    }
    return JS_NewBool(ctx, host->context->window->isMouseButtonActive(
                               static_cast<MouseButton>(button)));
}

JSValue jsIsMouseButtonPressed(JSContext *ctx, JSValueConst, int argc,
                               JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    std::int64_t button = 0;
    if (!getInt64(ctx, argv[0], button)) {
        return JS_ThrowTypeError(ctx, "Expected mouse button");
    }
    return JS_NewBool(ctx, host->context->window->isMouseButtonPressed(
                               static_cast<MouseButton>(button)));
}

JSValue jsGetTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_NewString(ctx, "");
    }
    return JS_NewString(ctx, host->context->window->getTextInput().c_str());
}

JSValue jsStartTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->startTextInput();
    }
    return JS_UNDEFINED;
}

JSValue jsStopTextInput(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->stopTextInput();
    }
    return JS_UNDEFINED;
}

JSValue jsIsTextInputActive(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return JS_FALSE;
    }
    return JS_NewBool(ctx, host->context->window->isTextInputActive());
}

JSValue jsIsControllerButtonPressed(JSContext *ctx, JSValueConst, int argc,
                                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_FALSE;
    }

    std::int64_t controllerID = 0;
    std::int64_t buttonIndex = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], buttonIndex)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected controller id and button index");
    }

    return JS_NewBool(ctx, host->context->window->isControllerButtonPressed(
                               static_cast<int>(controllerID),
                               static_cast<int>(buttonIndex)));
}

JSValue jsGetControllerAxisValue(JSContext *ctx, JSValueConst, int argc,
                                 JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 2) {
        return JS_NewFloat64(ctx, 0.0);
    }

    std::int64_t controllerID = 0;
    std::int64_t axisIndex = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], axisIndex)) {
        return JS_ThrowTypeError(ctx, "Expected controller id and axis index");
    }

    return JS_NewFloat64(
        ctx, host->context->window->getControllerAxisValue(
                 static_cast<int>(controllerID), static_cast<int>(axisIndex)));
}

JSValue jsGetControllerAxisPairValue(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 3) {
        return makePlainPosition2d(ctx, Position2d(0.0, 0.0));
    }

    std::int64_t controllerID = 0;
    std::int64_t axisIndexX = 0;
    std::int64_t axisIndexY = 0;
    if (!getInt64(ctx, argv[0], controllerID) ||
        !getInt64(ctx, argv[1], axisIndexX) ||
        !getInt64(ctx, argv[2], axisIndexY)) {
        return JS_ThrowTypeError(ctx,
                                 "Expected controller id and axis indices");
    }

    const auto value = host->context->window->getControllerAxisPairValue(
        static_cast<int>(controllerID), static_cast<int>(axisIndexX),
        static_cast<int>(axisIndexY));
    return makePlainPosition2d(ctx, Position2d(value.first, value.second));
}

JSValue jsCaptureMouse(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->captureMouse();
    }
    return JS_UNDEFINED;
}

JSValue jsReleaseMouse(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host != nullptr && host->context != nullptr &&
        host->context->window != nullptr) {
        host->context->window->releaseMouse();
    }
    return JS_UNDEFINED;
}

JSValue jsGetMousePosition(JSContext *ctx, JSValueConst, int, JSValueConst *) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr) {
        return makePlainPosition2d(ctx, Position2d(0.0, 0.0));
    }

    const auto [x, y] = host->context->window->getCursorPosition();
    return makePlainPosition2d(ctx, Position2d(x, y));
}

JSValue jsIsActionTriggered(JSContext *ctx, JSValueConst, int argc,
                            JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const bool result = host->context->window->isActionTriggered(name);
    JS_FreeCString(ctx, name);
    return JS_NewBool(ctx, result);
}

JSValue jsIsActionCurrentlyActive(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return JS_FALSE;
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const bool result = host->context->window->isActionCurrentlyActive(name);
    JS_FreeCString(ctx, name);
    return JS_NewBool(ctx, result);
}

JSValue jsGetAxisActionValue(JSContext *ctx, JSValueConst, int argc,
                             JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr ||
        host->context->window == nullptr || argc < 1) {
        return makeAxisPacketValue(ctx, {});
    }

    const char *name = JS_ToCString(ctx, argv[0]);
    if (name == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected action name");
    }
    const AxisPacket packet = host->context->window->getAxisActionValue(name);
    JS_FreeCString(ctx, name);
    return makeAxisPacketValue(ctx, packet);
}

JSValue jsRegisterInteractive(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1 || !JS_IsObject(argv[0])) {
        return JS_ThrowTypeError(ctx, "Expected interactive object");
    }

    for (const JSValue &interactive : host->interactiveValues) {
        if (JS_IsStrictEqual(ctx, interactive, argv[0])) {
            return JS_DupValue(ctx, argv[0]);
        }
    }

    host->interactiveValues.push_back(JS_DupValue(ctx, argv[0]));
    return JS_DupValue(ctx, argv[0]);
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
            return JS_ThrowReferenceError(ctx,
                                          "AudioPlayer wrapper is invalid");
        }

        auto *audioState = findAudioPlayerState(
            *host, static_cast<std::uint64_t>(audioPlayerId));
        if (audioState == nullptr || !audioState->component) {
            return JS_ThrowReferenceError(ctx, "Unknown audio player id");
        }

        if (audioState->attached) {
            return JS_ThrowTypeError(
                ctx, "AudioPlayer is already attached to an object");
        }

        object->addComponent(audioState->component);
        runtime::scripting::registerComponentInstance(
            ctx, *host, audioState->component.get(), static_cast<int>(ownerId),
            "AudioPlayer", argv[1]);
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

    auto component = std::make_shared<HostedScriptComponent>(ctx, host, argv[1],
                                                             componentName);
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
                                          .attachedToWindow = attachedToWindow,
                                          .textureIds = {}};
    return object;
}

std::shared_ptr<Model> ownModel(ScriptHost &host, std::shared_ptr<Model> model,
                                bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(model);
    }
    host.objectStates[model->getId()] = {.object = model.get(),
                                         .attachedToWindow = attachedToWindow,
                                         .textureIds = {}};
    return model;
}

std::shared_ptr<ParticleEmitter>
ownParticleEmitter(ScriptHost &host, std::shared_ptr<ParticleEmitter> emitter,
                   bool attachedToWindow = false) {
    if (host.context != nullptr) {
        host.context->objects.push_back(emitter);
    }
    host.objectStates[emitter->getId()] = {.object = emitter.get(),
                                           .attachedToWindow = attachedToWindow,
                                           .textureIds = {}};
    return emitter;
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

JSValue jsCreateModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected model resource path");
    }

    const char *path = JS_ToCString(ctx, argv[0]);
    if (path == nullptr) {
        return JS_ThrowTypeError(ctx, "Expected model resource path");
    }

    std::filesystem::path resourcePath(path);
    JS_FreeCString(ctx, path);

    std::string name = resourcePath.stem().string();
    if (name.empty()) {
        name = resourcePath.filename().string();
    }

    Resource resource = Workspace::get().createResource(resourcePath, name,
                                                        ResourceType::Model);
    auto object = ownModel(*host, std::make_shared<Model>());
    object->fromResource(resource);
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsGetModelObjects(JSContext *ctx, JSValueConst, int argc,
                          JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected model");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    JSValue result = JS_NewArray(ctx);
    const auto &objects = model->getObjects();
    for (std::uint32_t i = 0; i < objects.size(); ++i) {
        const auto &object = objects[i];
        if (object == nullptr) {
            JS_SetPropertyUint32(ctx, result, i, JS_NULL);
            continue;
        }

        auto &state = host->objectStates[object->getId()];
        state.object = object.get();
        state.attachedToWindow = true;

        JS_SetPropertyUint32(ctx, result, i,
                             syncObjectWrapper(ctx, *host, *object));
    }

    return result;
}

JSValue jsMoveModel(JSContext *ctx, JSValueConst, int argc,
                    JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and position");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    model->move(position);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelPosition(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and position");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    model->setPosition(position);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelRotation(JSContext *ctx, JSValueConst, int argc,
                           JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and rotation");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    model->setRotation(rotation);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsLookAtModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and target");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
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

    model->lookAt(target, up);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsRotateModel(JSContext *ctx, JSValueConst, int argc,
                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and rotation");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Rotation3d rotation;
    if (!parseRotation3d(ctx, argv[1], rotation)) {
        return JS_ThrowTypeError(ctx, "Expected Rotation3d");
    }

    model->rotate(rotation);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsSetModelScale(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and scale");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Scale3d scale;
    if (!parsePosition3d(ctx, argv[1], scale)) {
        return JS_ThrowTypeError(ctx, "Expected Scale3d");
    }

    model->setScale(scale);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsScaleModelBy(JSContext *ctx, JSValueConst, int argc,
                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected model and scale");
    }

    Model *model = resolveModelArg(ctx, *host, argv[0]);
    if (model == nullptr) {
        return JS_EXCEPTION;
    }

    Scale3d scale;
    if (!parsePosition3d(ctx, argv[1], scale)) {
        return JS_ThrowTypeError(ctx, "Expected Scale3d");
    }

    Scale3d currentScale = model->getScale();
    currentScale.x *= scale.x;
    currentScale.y *= scale.y;
    currentScale.z *= scale.z;
    model->setScale(currentScale);
    return syncObjectWrapper(ctx, *host, *model);
}

JSValue jsCreateParticleEmitter(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and max particles");
    }

    std::int64_t maxParticles = 0;
    if (!getInt64(ctx, argv[1], maxParticles)) {
        return JS_ThrowTypeError(ctx, "Expected max particles");
    }

    auto emitter =
        ownParticleEmitter(*host, std::make_shared<ParticleEmitter>(
                                      static_cast<unsigned int>(maxParticles)));

    JSValue objectValue = JS_DupValue(ctx, argv[0]);
    setProperty(ctx, objectValue, ATLAS_PARTICLE_EMITTER_ID_PROP,
                JS_NewInt32(ctx, emitter->getId()));
    setProperty(ctx, objectValue, "id", JS_NewInt32(ctx, emitter->getId()));
    setProperty(ctx, objectValue, ATLAS_GENERATION_PROP,
                JS_NewInt64(ctx, static_cast<int64_t>(host->generation)));
    JS_FreeValue(ctx, objectValue);

    attachObjectIfReady(*host, *emitter);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsAttachParticleEmitterTexture(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and texture");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    auto *textureState = resolveTexture(ctx, *host, argv[1]);
    if (textureState == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->attachTexture(*textureState->texture);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterColor(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and color");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Color color;
    if (!parseColor(ctx, argv[1], color)) {
        return JS_ThrowTypeError(ctx, "Expected Color");
    }

    emitter->setColor(color);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterUseTexture(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and enabled flag");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    if (JS_ToBool(ctx, argv[1]) == 1) {
        emitter->enableTexture();
    } else {
        emitter->disableTexture();
    }

    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterPosition(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and position");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->setPosition(position);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsMoveParticleEmitter(JSContext *ctx, JSValueConst, int argc,
                              JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and position");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d position;
    if (!parsePosition3d(ctx, argv[1], position)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->move(position);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsGetParticleEmitterPosition(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    return makePosition3d(ctx, *host, emitter->getPosition());
}

JSValue jsSetParticleEmitterEmissionType(JSContext *ctx, JSValueConst, int argc,
                                         JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and emission type");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t type = 0;
    if (!getInt64(ctx, argv[1], type)) {
        return JS_ThrowTypeError(ctx, "Expected emission type");
    }

    emitter->setEmissionType(type == 1 ? ParticleEmissionType::Ambient
                                       : ParticleEmissionType::Fountain);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterDirection(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and direction");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    Position3d direction;
    if (!parsePosition3d(ctx, argv[1], direction)) {
        return JS_ThrowTypeError(ctx, "Expected Position3d");
    }

    emitter->setDirection(direction);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSpawnRadius(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and spawn radius");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    double radius = 0.0;
    if (!getDouble(ctx, argv[1], radius)) {
        return JS_ThrowTypeError(ctx, "Expected spawn radius");
    }

    emitter->setSpawnRadius(static_cast<float>(radius));
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSpawnRate(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx,
                                 "Expected particle emitter and spawn rate");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    double rate = 0.0;
    if (!getDouble(ctx, argv[1], rate)) {
        return JS_ThrowTypeError(ctx, "Expected spawn rate");
    }

    emitter->setSpawnRate(static_cast<float>(rate));
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsSetParticleEmitterSettings(JSContext *ctx, JSValueConst, int argc,
                                     JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and settings");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    ParticleSettings settings = emitter->settings;
    parseParticleSettings(ctx, argv[1], settings);
    emitter->setParticleSettings(settings);
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitOnce(JSContext *ctx, JSValueConst, int argc,
                                  JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->emitOnce();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitContinuous(JSContext *ctx, JSValueConst, int argc,
                                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->emitContinuously();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterStartEmission(JSContext *ctx, JSValueConst, int argc,
                                       JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->startEmission();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterStopEmission(JSContext *ctx, JSValueConst, int argc,
                                      JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 1) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    emitter->stopEmission();
    return syncObjectWrapper(ctx, *host, *emitter);
}

JSValue jsParticleEmitterEmitBurst(JSContext *ctx, JSValueConst, int argc,
                                   JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected particle emitter and count");
    }

    ParticleEmitter *emitter = resolveParticleEmitterArg(ctx, *host, argv[0]);
    if (emitter == nullptr) {
        return JS_EXCEPTION;
    }

    std::int64_t count = 0;
    if (!getInt64(ctx, argv[1], count)) {
        return JS_ThrowTypeError(ctx, "Expected burst count");
    }

    emitter->emitBurst(static_cast<int>(count));
    return syncObjectWrapper(ctx, *host, *emitter);
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

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createPyramid(size)));
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
}

JSValue jsCreatePrimitiveSphere(JSContext *ctx, JSValueConst, int argc,
                                JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || host->context == nullptr || argc < 3) {
        return JS_ThrowTypeError(
            ctx, "Expected radius, sectorCount, and stackCount");
    }

    double radius = 0.0;
    std::int64_t sectorCount = 0;
    std::int64_t stackCount = 0;
    if (!getDouble(ctx, argv[0], radius) ||
        !getInt64(ctx, argv[1], sectorCount) ||
        !getInt64(ctx, argv[2], stackCount)) {
        return JS_ThrowTypeError(
            ctx, "Expected radius, sectorCount, and stackCount");
    }

    auto object =
        ownCoreObject(*host, std::make_shared<CoreObject>(createSphere(
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
    if (!getInt64(ctx, argv[0], objectId) ||
        !getDouble(ctx, argv[2], intensity)) {
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

JSValue jsAttachTexture(JSContext *ctx, JSValueConst, int argc,
                        JSValueConst *argv) {
    auto *host = getHost(ctx);
    if (host == nullptr || argc < 2) {
        return JS_ThrowTypeError(ctx, "Expected object and texture");
    }

    GameObject *object = resolveObjectArg(ctx, *host, argv[0]);
    if (object == nullptr) {
        return JS_EXCEPTION;
    }

    auto *textureState = resolveTexture(ctx, *host, argv[1]);
    if (textureState == nullptr) {
        return JS_EXCEPTION;
    }

    object->attachTexture(*textureState->texture);
    attachObjectIfReady(*host, *object);
    return syncObjectWrapper(ctx, *host, *object);
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
    return syncInstanceWrapper(
        ctx, *host, *object,
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
    if (!JS_IsUndefined(host.cameraValue)) {
        JS_FreeValue(ctx, host.cameraValue);
        host.cameraValue = JS_UNDEFINED;
    }
    if (!JS_IsUndefined(host.sceneValue)) {
        JS_FreeValue(ctx, host.sceneValue);
        host.sceneValue = JS_UNDEFINED;
    }

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

    for (auto &[_, state] : host.textures) {
        JS_FreeValue(ctx, state.value);
    }
    host.textures.clear();

    for (auto &[_, state] : host.cubemaps) {
        JS_FreeValue(ctx, state.value);
    }
    host.cubemaps.clear();

    for (auto &[_, state] : host.skyboxes) {
        JS_FreeValue(ctx, state.value);
    }
    host.skyboxes.clear();

    for (auto &[_, state] : host.renderTargets) {
        JS_FreeValue(ctx, state.value);
    }
    host.renderTargets.clear();

    for (auto &[_, state] : host.pointLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.pointLights.clear();

    for (auto &[_, state] : host.directionalLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.directionalLights.clear();

    for (auto &[_, state] : host.spotLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.spotLights.clear();

    for (auto &[_, state] : host.areaLights) {
        JS_FreeValue(ctx, state.value);
    }
    host.areaLights.clear();

    if (!JS_IsUndefined(host.atlasParticleNamespace)) {
        JS_FreeValue(ctx, host.atlasParticleNamespace);
        host.atlasParticleNamespace = JS_UNDEFINED;
    }

    if (!JS_IsUndefined(host.particleEmitterPrototype)) {
        JS_FreeValue(ctx, host.particleEmitterPrototype);
        host.particleEmitterPrototype = JS_UNDEFINED;
    }

    for (JSValue &interactive : host.interactiveValues) {
        JS_FreeValue(ctx, interactive);
    }
    host.interactiveValues.clear();

    host.componentIds.clear();
    host.componentOrder.clear();
    host.componentLookup.clear();
    host.interactiveKeyStates.clear();
    host.interactiveFirstMouse = true;
    host.objectStates.clear();
    host.nextComponentId = 1;
    host.nextAudioPlayerId = 1;
    host.nextTextureId = 1;
    host.nextCubemapId = 1;
    host.nextSkyboxId = 1;
    host.nextRenderTargetId = 1;
    host.nextPointLightId = 1;
    host.nextDirectionalLightId = 1;
    host.nextSpotLightId = 1;
    host.nextAreaLightId = 1;
    host.generation += 1;
}

void runtime::scripting::dispatchInteractiveFrame(JSContext *ctx,
                                                  ScriptHost &host,
                                                  Window &window,
                                                  float deltaTime) {
    if (host.interactiveValues.empty()) {
        host.interactiveKeyStates.clear();
        return;
    }

    auto dispatch = [&](const char *methodName, int argc, JSValue *args) {
        JSValueConst *constArgs = args;
        for (JSValue &interactive : host.interactiveValues) {
            callObjectMethod(ctx, interactive, methodName, argc, constArgs);
        }
        for (int i = 0; i < argc; ++i) {
            JS_FreeValue(ctx, args[i]);
        }
    };

    for (const auto &[_, key] : ATLAS_KEY_ENTRIES) {
        const int keyCode = static_cast<int>(key);
        const bool active = window.isKeyActive(key);
        const bool previous = host.interactiveKeyStates[keyCode];
        if (active && !previous) {
            JSValue args[] = {JS_NewInt32(ctx, keyCode),
                              JS_NewFloat64(ctx, deltaTime)};
            dispatch("onKeyPress", 2, args);
        } else if (!active && previous) {
            JSValue args[] = {JS_NewInt32(ctx, keyCode),
                              JS_NewFloat64(ctx, deltaTime)};
            dispatch("onKeyRelease", 2, args);
        }
        host.interactiveKeyStates[keyCode] = active;
    }

    for (MouseButton button : ATLAS_MOUSE_BUTTONS) {
        if (!window.isMouseButtonPressed(button)) {
            continue;
        }
        JSValue args[] = {JS_NewInt32(ctx, static_cast<int>(button)),
                          JS_NewFloat64(ctx, deltaTime)};
        dispatch("onMouseButtonPress", 2, args);
    }

    JSValue frameArgs[] = {JS_NewFloat64(ctx, deltaTime)};
    dispatch("onEachFrame", 1, frameArgs);
}

void runtime::scripting::dispatchInteractiveMouseMove(JSContext *ctx,
                                                      ScriptHost &host,
                                                      Window &window,
                                                      const MousePacket &packet,
                                                      float deltaTime) {
    if (host.interactiveValues.empty()) {
        return;
    }

    JSValue args[] = {makeMousePacketValue(ctx, packet),
                      JS_NewFloat64(ctx, deltaTime)};
    JSValueConst *constArgs = args;
    for (JSValue &interactive : host.interactiveValues) {
        callObjectMethod(ctx, interactive, "onMouseMove", 2, constArgs);
    }
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);

    (void)window;
    host.interactiveFirstMouse = false;
}

void runtime::scripting::dispatchInteractiveMouseScroll(
    JSContext *ctx, ScriptHost &host, const MouseScrollPacket &packet,
    float deltaTime) {
    if (host.interactiveValues.empty()) {
        return;
    }

    JSValue args[] = {makeMouseScrollPacketValue(ctx, packet),
                      JS_NewFloat64(ctx, deltaTime)};
    JSValueConst *constArgs = args;
    for (JSValue &interactive : host.interactiveValues) {
        callObjectMethod(ctx, interactive, "onMouseScroll", 2, constArgs);
    }
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
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
    JS_SetPropertyStr(ctx, global, "__atlasGetScene",
                      JS_NewCFunction(ctx, jsGetScene, "__atlasGetScene", 0));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneAmbientIntensity",
                      JS_NewCFunction(ctx, jsSetSceneAmbientIntensity,
                                      "__atlasSetSceneAmbientIntensity", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetSceneAutomaticAmbient",
                      JS_NewCFunction(ctx, jsSetSceneAutomaticAmbient,
                                      "__atlasSetSceneAutomaticAmbient", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetSceneSkybox",
        JS_NewCFunction(ctx, jsSetSceneSkybox, "__atlasSetSceneSkybox", 2));
    JS_SetPropertyStr(ctx, global, "__atlasUseAtmosphereSkybox",
                      JS_NewCFunction(ctx, jsUseAtmosphereSkybox,
                                      "__atlasUseAtmosphereSkybox", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetCamera",
                      JS_NewCFunction(ctx, jsGetCamera, "__atlasGetCamera", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateCamera",
        JS_NewCFunction(ctx, jsUpdateCamera, "__atlasUpdateCamera", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetPositionKeepingOrientation",
                      JS_NewCFunction(ctx, jsSetPositionKeepingOrientation,
                                      "__atlasSetPositionKeepingOrientation",
                                      2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtCamera",
        JS_NewCFunction(ctx, jsLookAtCamera, "__atlasLookAtCamera", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasMoveCameraTo",
        JS_NewCFunction(ctx, jsMoveCameraTo, "__atlasMoveCameraTo", 3));
    JS_SetPropertyStr(ctx, global, "__atlasGetCameraDirection",
                      JS_NewCFunction(ctx, jsGetCameraDirection,
                                      "__atlasGetCameraDirection", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetInputConstants",
                      JS_NewCFunction(ctx, jsGetInputConstants,
                                      "__atlasGetInputConstants", 0));
    JS_SetPropertyStr(ctx, global, "__atlasRegisterInputAction",
                      JS_NewCFunction(ctx, jsRegisterInputAction,
                                      "__atlasRegisterInputAction", 1));
    JS_SetPropertyStr(ctx, global, "__atlasResetInputActions",
                      JS_NewCFunction(ctx, jsResetInputActions,
                                      "__atlasResetInputActions", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasIsKeyActive",
        JS_NewCFunction(ctx, jsIsKeyActive, "__atlasIsKeyActive", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasIsKeyPressed",
        JS_NewCFunction(ctx, jsIsKeyPressed, "__atlasIsKeyPressed", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsMouseButtonActive",
                      JS_NewCFunction(ctx, jsIsMouseButtonActive,
                                      "__atlasIsMouseButtonActive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsMouseButtonPressed",
                      JS_NewCFunction(ctx, jsIsMouseButtonPressed,
                                      "__atlasIsMouseButtonPressed", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetTextInput",
        JS_NewCFunction(ctx, jsGetTextInput, "__atlasGetTextInput", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasStartTextInput",
        JS_NewCFunction(ctx, jsStartTextInput, "__atlasStartTextInput", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasStopTextInput",
        JS_NewCFunction(ctx, jsStopTextInput, "__atlasStopTextInput", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsTextInputActive",
                      JS_NewCFunction(ctx, jsIsTextInputActive,
                                      "__atlasIsTextInputActive", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsControllerButtonPressed",
                      JS_NewCFunction(ctx, jsIsControllerButtonPressed,
                                      "__atlasIsControllerButtonPressed", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetControllerAxisValue",
                      JS_NewCFunction(ctx, jsGetControllerAxisValue,
                                      "__atlasGetControllerAxisValue", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetControllerAxisPairValue",
                      JS_NewCFunction(ctx, jsGetControllerAxisPairValue,
                                      "__atlasGetControllerAxisPairValue", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasCaptureMouse",
        JS_NewCFunction(ctx, jsCaptureMouse, "__atlasCaptureMouse", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasReleaseMouse",
        JS_NewCFunction(ctx, jsReleaseMouse, "__atlasReleaseMouse", 0));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetMousePosition",
        JS_NewCFunction(ctx, jsGetMousePosition, "__atlasGetMousePosition", 0));
    JS_SetPropertyStr(ctx, global, "__atlasIsActionTriggered",
                      JS_NewCFunction(ctx, jsIsActionTriggered,
                                      "__atlasIsActionTriggered", 1));
    JS_SetPropertyStr(ctx, global, "__atlasIsActionCurrentlyActive",
                      JS_NewCFunction(ctx, jsIsActionCurrentlyActive,
                                      "__atlasIsActionCurrentlyActive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetAxisActionValue",
                      JS_NewCFunction(ctx, jsGetAxisActionValue,
                                      "__atlasGetAxisActionValue", 1));
    JS_SetPropertyStr(ctx, global, "__atlasRegisterInteractive",
                      JS_NewCFunction(ctx, jsRegisterInteractive,
                                      "__atlasRegisterInteractive", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateTextureFromResource",
                      JS_NewCFunction(ctx, jsCreateTextureFromResource,
                                      "__atlasCreateTextureFromResource", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateEmptyTexture",
                      JS_NewCFunction(ctx, jsCreateEmptyTexture,
                                      "__atlasCreateEmptyTexture", 4));
    JS_SetPropertyStr(ctx, global, "__atlasCreateColorTexture",
                      JS_NewCFunction(ctx, jsCreateColorTexture,
                                      "__atlasCreateColorTexture", 4));
    JS_SetPropertyStr(ctx, global, "__atlasCreateCheckerboardTexture",
                      JS_NewCFunction(ctx, jsCreateCheckerboardTexture,
                                      "__atlasCreateCheckerboardTexture", 6));
    JS_SetPropertyStr(ctx, global, "__atlasCreateDoubleCheckerboardTexture",
                      JS_NewCFunction(ctx, jsCreateDoubleCheckerboardTexture,
                                      "__atlasCreateDoubleCheckerboardTexture",
                                      8));
    JS_SetPropertyStr(
        ctx, global, "__atlasDisplayTexture",
        JS_NewCFunction(ctx, jsDisplayTexture, "__atlasDisplayTexture", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateCubemap",
        JS_NewCFunction(ctx, jsCreateCubemap, "__atlasCreateCubemap", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateCubemapFromGroup",
                      JS_NewCFunction(ctx, jsCreateCubemapFromGroup,
                                      "__atlasCreateCubemapFromGroup", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetCubemapAverageColor",
                      JS_NewCFunction(ctx, jsGetCubemapAverageColor,
                                      "__atlasGetCubemapAverageColor", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateCubemapWithColors",
                      JS_NewCFunction(ctx, jsUpdateCubemapWithColors,
                                      "__atlasUpdateCubemapWithColors", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateSkybox",
        JS_NewCFunction(ctx, jsCreateSkybox, "__atlasCreateSkybox", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateRenderTarget",
                      JS_NewCFunction(ctx, jsCreateRenderTarget,
                                      "__atlasCreateRenderTarget", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAddRenderTargetEffect",
                      JS_NewCFunction(ctx, jsAddRenderTargetEffect,
                                      "__atlasAddRenderTargetEffect", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAddRenderTargetToPassQueue",
                      JS_NewCFunction(ctx, jsAddRenderTargetToPassQueue,
                                      "__atlasAddRenderTargetToPassQueue", 2));
    JS_SetPropertyStr(ctx, global, "__atlasDisplayRenderTarget",
                      JS_NewCFunction(ctx, jsDisplayRenderTarget,
                                      "__atlasDisplayRenderTarget", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreatePointLight",
        JS_NewCFunction(ctx, jsCreatePointLight, "__atlasCreatePointLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdatePointLight",
        JS_NewCFunction(ctx, jsUpdatePointLight, "__atlasUpdatePointLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePointLightDebugObject",
                      JS_NewCFunction(ctx, jsCreatePointLightDebugObject,
                                      "__atlasCreatePointLightDebugObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastPointLightShadows",
                      JS_NewCFunction(ctx, jsCastPointLightShadows,
                                      "__atlasCastPointLightShadows", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateDirectionalLight",
                      JS_NewCFunction(ctx, jsCreateDirectionalLight,
                                      "__atlasCreateDirectionalLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasUpdateDirectionalLight",
                      JS_NewCFunction(ctx, jsUpdateDirectionalLight,
                                      "__atlasUpdateDirectionalLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastDirectionalLightShadows",
                      JS_NewCFunction(ctx, jsCastDirectionalLightShadows,
                                      "__atlasCastDirectionalLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateSpotLight",
        JS_NewCFunction(ctx, jsCreateSpotLight, "__atlasCreateSpotLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateSpotLight",
        JS_NewCFunction(ctx, jsUpdateSpotLight, "__atlasUpdateSpotLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSpotLightDebugObject",
                      JS_NewCFunction(ctx, jsCreateSpotLightDebugObject,
                                      "__atlasCreateSpotLightDebugObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtSpotLight",
        JS_NewCFunction(ctx, jsLookAtSpotLight, "__atlasLookAtSpotLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCastSpotLightShadows",
                      JS_NewCFunction(ctx, jsCastSpotLightShadows,
                                      "__atlasCastSpotLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateAreaLight",
        JS_NewCFunction(ctx, jsCreateAreaLight, "__atlasCreateAreaLight", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateAreaLight",
        JS_NewCFunction(ctx, jsUpdateAreaLight, "__atlasUpdateAreaLight", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetAreaLightNormal",
                      JS_NewCFunction(ctx, jsGetAreaLightNormal,
                                      "__atlasGetAreaLightNormal", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetAreaLightRotation",
                      JS_NewCFunction(ctx, jsSetAreaLightRotation,
                                      "__atlasSetAreaLightRotation", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasRotateAreaLight",
        JS_NewCFunction(ctx, jsRotateAreaLight, "__atlasRotateAreaLight", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateAreaLightDebugObject",
                      JS_NewCFunction(ctx, jsCreateAreaLightDebugObject,
                                      "__atlasCreateAreaLightDebugObject", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCastAreaLightShadows",
                      JS_NewCFunction(ctx, jsCastAreaLightShadows,
                                      "__atlasCastAreaLightShadows", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetObjectById",
        JS_NewCFunction(ctx, jsGetObjectById, "__atlasGetObjectById", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetObjectByName",
        JS_NewCFunction(ctx, jsGetObjectByName, "__atlasGetObjectByName", 1));
    JS_SetPropertyStr(ctx, global, "__atlasGetComponentByName",
                      JS_NewCFunction(ctx, jsGetComponentByName,
                                      "__atlasGetComponentByName", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLoadResource",
        JS_NewCFunction(ctx, jsLoadResource, "__atlasLoadResource", 3));
    JS_SetPropertyStr(ctx, global, "__atlasGetResourceByName",
                      JS_NewCFunction(ctx, jsGetResourceByName,
                                      "__atlasGetResourceByName", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasAddComponent",
        JS_NewCFunction(ctx, jsAddComponent, "__atlasAddComponent", 2));
    JS_SetPropertyStr(ctx, global, "__atlasCreateAudioPlayer",
                      JS_NewCFunction(ctx, jsCreateAudioPlayer,
                                      "__atlasCreateAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasInitAudioPlayer",
        JS_NewCFunction(ctx, jsInitAudioPlayer, "__atlasInitAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasPlayAudioPlayer",
        JS_NewCFunction(ctx, jsPlayAudioPlayer, "__atlasPlayAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasPauseAudioPlayer",
        JS_NewCFunction(ctx, jsPauseAudioPlayer, "__atlasPauseAudioPlayer", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasStopAudioPlayer",
        JS_NewCFunction(ctx, jsStopAudioPlayer, "__atlasStopAudioPlayer", 1));
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
    JS_SetPropertyStr(
        ctx, global, "__atlasUseSpatialAudio",
        JS_NewCFunction(ctx, jsUseSpatialAudio, "__atlasUseSpatialAudio", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateCoreObject",
        JS_NewCFunction(ctx, jsCreateCoreObject, "__atlasCreateCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateModel",
        JS_NewCFunction(ctx, jsCreateModel, "__atlasCreateModel", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasGetModelObjects",
        JS_NewCFunction(ctx, jsGetModelObjects, "__atlasGetModelObjects", 1));
    JS_SetPropertyStr(ctx, global, "__atlasMoveModel",
                      JS_NewCFunction(ctx, jsMoveModel, "__atlasMoveModel", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelPosition",
        JS_NewCFunction(ctx, jsSetModelPosition, "__atlasSetModelPosition", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelRotation",
        JS_NewCFunction(ctx, jsSetModelRotation, "__atlasSetModelRotation", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtModel",
        JS_NewCFunction(ctx, jsLookAtModel, "__atlasLookAtModel", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasRotateModel",
        JS_NewCFunction(ctx, jsRotateModel, "__atlasRotateModel", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasSetModelScale",
        JS_NewCFunction(ctx, jsSetModelScale, "__atlasSetModelScale", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasScaleModelBy",
        JS_NewCFunction(ctx, jsScaleModelBy, "__atlasScaleModelBy", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowModel",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowModel", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideModel",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideModel", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateParticleEmitter",
                      JS_NewCFunction(ctx, jsCreateParticleEmitter,
                                      "__atlasCreateParticleEmitter", 2));
    JS_SetPropertyStr(ctx, global, "__atlasAttachParticleEmitterTexture",
                      JS_NewCFunction(ctx, jsAttachParticleEmitterTexture,
                                      "__atlasAttachParticleEmitterTexture",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterColor",
                      JS_NewCFunction(ctx, jsSetParticleEmitterColor,
                                      "__atlasSetParticleEmitterColor", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterUseTexture",
                      JS_NewCFunction(ctx, jsSetParticleEmitterUseTexture,
                                      "__atlasSetParticleEmitterUseTexture",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterPosition",
                      JS_NewCFunction(ctx, jsSetParticleEmitterPosition,
                                      "__atlasSetParticleEmitterPosition", 2));
    JS_SetPropertyStr(ctx, global, "__atlasMoveParticleEmitter",
                      JS_NewCFunction(ctx, jsMoveParticleEmitter,
                                      "__atlasMoveParticleEmitter", 2));
    JS_SetPropertyStr(ctx, global, "__atlasGetParticleEmitterPosition",
                      JS_NewCFunction(ctx, jsGetParticleEmitterPosition,
                                      "__atlasGetParticleEmitterPosition", 1));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterEmissionType",
                      JS_NewCFunction(ctx, jsSetParticleEmitterEmissionType,
                                      "__atlasSetParticleEmitterEmissionType",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterDirection",
                      JS_NewCFunction(ctx, jsSetParticleEmitterDirection,
                                      "__atlasSetParticleEmitterDirection", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSpawnRadius",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSpawnRadius,
                                      "__atlasSetParticleEmitterSpawnRadius",
                                      2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSpawnRate",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSpawnRate,
                                      "__atlasSetParticleEmitterSpawnRate", 2));
    JS_SetPropertyStr(ctx, global, "__atlasSetParticleEmitterSettings",
                      JS_NewCFunction(ctx, jsSetParticleEmitterSettings,
                                      "__atlasSetParticleEmitterSettings", 2));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitOnce",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitOnce,
                                      "__atlasParticleEmitterEmitOnce", 1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitContinuous",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitContinuous,
                                      "__atlasParticleEmitterEmitContinuous",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterStartEmission",
                      JS_NewCFunction(ctx, jsParticleEmitterStartEmission,
                                      "__atlasParticleEmitterStartEmission",
                                      1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterStopEmission",
                      JS_NewCFunction(ctx, jsParticleEmitterStopEmission,
                                      "__atlasParticleEmitterStopEmission", 1));
    JS_SetPropertyStr(ctx, global, "__atlasParticleEmitterEmitBurst",
                      JS_NewCFunction(ctx, jsParticleEmitterEmitBurst,
                                      "__atlasParticleEmitterEmitBurst", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateObject",
        JS_NewCFunction(ctx, jsUpdateObject, "__atlasUpdateObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasUpdateCoreObject",
        JS_NewCFunction(ctx, jsUpdateObject, "__atlasUpdateCoreObject", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateBox",
        JS_NewCFunction(ctx, jsCreatePrimitiveBox, "__atlasCreateBox", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreatePlane",
        JS_NewCFunction(ctx, jsCreatePrimitivePlane, "__atlasCreatePlane", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreatePyramid",
                      JS_NewCFunction(ctx, jsCreatePrimitivePyramid,
                                      "__atlasCreatePyramid", 1));
    JS_SetPropertyStr(ctx, global, "__atlasCreateSphere",
                      JS_NewCFunction(ctx, jsCreatePrimitiveSphere,
                                      "__atlasCreateSphere", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasCloneCoreObject",
        JS_NewCFunction(ctx, jsCloneCoreObject, "__atlasCloneCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasMakeEmissive",
        JS_NewCFunction(ctx, jsMakeEmissive, "__atlasMakeEmissive", 3));
    JS_SetPropertyStr(
        ctx, global, "__atlasAttachTexture",
        JS_NewCFunction(ctx, jsAttachTexture, "__atlasAttachTexture", 2));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowObject",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideObject",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowCoreObject",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideCoreObject",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideCoreObject", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasShowParticleEmitter",
        JS_NewCFunction(ctx, jsShowObject, "__atlasShowParticleEmitter", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasHideParticleEmitter",
        JS_NewCFunction(ctx, jsHideObject, "__atlasHideParticleEmitter", 1));
    JS_SetPropertyStr(ctx, global, "__atlasEnableDeferredRendering",
                      JS_NewCFunction(ctx, jsEnableDeferred,
                                      "__atlasEnableDeferredRendering", 1));
    JS_SetPropertyStr(ctx, global, "__atlasDisableDeferredRendering",
                      JS_NewCFunction(ctx, jsDisableDeferred,
                                      "__atlasDisableDeferredRendering", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCreateInstance",
        JS_NewCFunction(ctx, jsCreateInstance, "__atlasCreateInstance", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasCommitInstance",
        JS_NewCFunction(ctx, jsCommitInstance, "__atlasCommitInstance", 1));
    JS_SetPropertyStr(
        ctx, global, "__atlasLookAtObject",
        JS_NewCFunction(ctx, jsLookAtObject, "__atlasLookAtObject", 3));
    JS_SetPropertyStr(ctx, global, "__atlasSetRotationQuaternion",
                      JS_NewCFunction(ctx, jsSetRotationQuaternion,
                                      "__atlasSetRotationQuaternion", 2));
    JS_FreeValue(ctx, global);
}

char *runtime::scripting::normalizeModuleName(JSContext *ctx,
                                              const char *baseName,
                                              const char *name, void *opaque) {
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

JSValue
runtime::scripting::importModuleNamespace(JSContext *ctx,
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
