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
class Model;
class Component;
class AudioPlayer;
class ParticleEmitter;
class Joint;
class FixedJoint;
class HingeJoint;
class SpringJoint;
class Vehicle;
class Rigidbody;
class Window;
struct Light;
class DirectionalLight;
struct Spotlight;
struct AreaLight;
struct Texture;
struct Cubemap;
class Skybox;
class RenderTarget;
struct MousePacket;
struct MouseScrollPacket;

struct ScriptObjectState {
    GameObject *object = nullptr;
    bool attachedToWindow = false;
    std::vector<std::uint64_t> textureIds;
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

struct ScriptRigidbodyState {
    std::shared_ptr<Rigidbody> ownedComponent;
    Rigidbody *component = nullptr;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptVehicleState {
    std::shared_ptr<Component> ownedComponent;
    Vehicle *component = nullptr;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptFixedJointState {
    std::shared_ptr<Component> ownedComponent;
    FixedJoint *component = nullptr;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptHingeJointState {
    std::shared_ptr<Component> ownedComponent;
    HingeJoint *component = nullptr;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptSpringJointState {
    std::shared_ptr<Component> ownedComponent;
    SpringJoint *component = nullptr;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
};

struct ScriptTextureState {
    std::shared_ptr<Texture> texture;
    JSValue value = JS_UNDEFINED;
};

struct ScriptCubemapState {
    std::shared_ptr<Cubemap> cubemap;
    JSValue value = JS_UNDEFINED;
};

struct ScriptSkyboxState {
    std::shared_ptr<Skybox> skybox;
    JSValue value = JS_UNDEFINED;
    std::uint64_t cubemapId = 0;
};

struct ScriptRenderTargetState {
    std::shared_ptr<RenderTarget> renderTarget;
    JSValue value = JS_UNDEFINED;
    bool attached = false;
    std::vector<std::uint64_t> outTextureIds;
    std::uint64_t depthTextureId = 0;
};

struct ScriptPointLightState {
    Light *light = nullptr;
    JSValue value = JS_UNDEFINED;
};

struct ScriptDirectionalLightState {
    DirectionalLight *light = nullptr;
    JSValue value = JS_UNDEFINED;
};

struct ScriptSpotLightState {
    Spotlight *light = nullptr;
    JSValue value = JS_UNDEFINED;
};

struct ScriptAreaLightState {
    AreaLight *light = nullptr;
    JSValue value = JS_UNDEFINED;
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
    std::unordered_map<std::uint64_t, ScriptRigidbodyState> rigidbodies;
    std::unordered_map<std::uint64_t, ScriptVehicleState> vehicles;
    std::unordered_map<std::uint64_t, ScriptFixedJointState> fixedJoints;
    std::unordered_map<std::uint64_t, ScriptHingeJointState> hingeJoints;
    std::unordered_map<std::uint64_t, ScriptSpringJointState> springJoints;
    std::unordered_map<std::uint64_t, ScriptTextureState> textures;
    std::unordered_map<std::uint64_t, ScriptCubemapState> cubemaps;
    std::unordered_map<std::uint64_t, ScriptSkyboxState> skyboxes;
    std::unordered_map<std::uint64_t, ScriptRenderTargetState> renderTargets;
    std::unordered_map<std::uint64_t, ScriptPointLightState> pointLights;
    std::unordered_map<std::uint64_t, ScriptDirectionalLightState>
        directionalLights;
    std::unordered_map<std::uint64_t, ScriptSpotLightState> spotLights;
    std::unordered_map<std::uint64_t, ScriptAreaLightState> areaLights;
    std::vector<JSValue> interactiveValues;
    std::unordered_map<int, bool> interactiveKeyStates;
    bool interactiveFirstMouse = true;
    JSValue windowValue = JS_UNDEFINED;
    JSValue cameraValue = JS_UNDEFINED;
    JSValue sceneValue = JS_UNDEFINED;
    JSValue atlasNamespace = JS_UNDEFINED;
    JSValue atlasInputNamespace = JS_UNDEFINED;
    JSValue atlasUnitsNamespace = JS_UNDEFINED;
    JSValue atlasGraphicsNamespace = JS_UNDEFINED;
    JSValue atlasParticleNamespace = JS_UNDEFINED;
    JSValue atlasBezelNamespace = JS_UNDEFINED;
    JSValue componentPrototype = JS_UNDEFINED;
    JSValue gameObjectPrototype = JS_UNDEFINED;
    JSValue coreObjectPrototype = JS_UNDEFINED;
    JSValue modelPrototype = JS_UNDEFINED;
    JSValue materialPrototype = JS_UNDEFINED;
    JSValue instancePrototype = JS_UNDEFINED;
    JSValue coreVertexPrototype = JS_UNDEFINED;
    JSValue resourcePrototype = JS_UNDEFINED;
    JSValue windowPrototype = JS_UNDEFINED;
    JSValue monitorPrototype = JS_UNDEFINED;
    JSValue gamepadPrototype = JS_UNDEFINED;
    JSValue joystickPrototype = JS_UNDEFINED;
    JSValue cameraPrototype = JS_UNDEFINED;
    JSValue scenePrototype = JS_UNDEFINED;
    JSValue texturePrototype = JS_UNDEFINED;
    JSValue cubemapPrototype = JS_UNDEFINED;
    JSValue skyboxPrototype = JS_UNDEFINED;
    JSValue renderTargetPrototype = JS_UNDEFINED;
    JSValue pointLightPrototype = JS_UNDEFINED;
    JSValue directionalLightPrototype = JS_UNDEFINED;
    JSValue spotLightPrototype = JS_UNDEFINED;
    JSValue areaLightPrototype = JS_UNDEFINED;
    JSValue particleEmitterPrototype = JS_UNDEFINED;
    JSValue rigidbodyPrototype = JS_UNDEFINED;
    JSValue sensorPrototype = JS_UNDEFINED;
    JSValue vehiclePrototype = JS_UNDEFINED;
    JSValue fixedJointPrototype = JS_UNDEFINED;
    JSValue hingeJointPrototype = JS_UNDEFINED;
    JSValue springJointPrototype = JS_UNDEFINED;
    JSValue position3dPrototype = JS_UNDEFINED;
    JSValue position2dPrototype = JS_UNDEFINED;
    JSValue colorPrototype = JS_UNDEFINED;
    JSValue size2dPrototype = JS_UNDEFINED;
    JSValue quaternionPrototype = JS_UNDEFINED;
    JSValue triggerPrototype = JS_UNDEFINED;
    JSValue axisTriggerPrototype = JS_UNDEFINED;
    JSValue inputActionPrototype = JS_UNDEFINED;
    std::uint64_t nextComponentId = 1;
    std::uint64_t nextAudioPlayerId = 1;
    std::uint64_t nextRigidbodyId = 1;
    std::uint64_t nextVehicleId = 1;
    std::uint64_t nextFixedJointId = 1;
    std::uint64_t nextHingeJointId = 1;
    std::uint64_t nextSpringJointId = 1;
    std::uint64_t nextTextureId = 1;
    std::uint64_t nextCubemapId = 1;
    std::uint64_t nextSkyboxId = 1;
    std::uint64_t nextRenderTargetId = 1;
    std::uint64_t nextPointLightId = 1;
    std::uint64_t nextDirectionalLightId = 1;
    std::uint64_t nextSpotLightId = 1;
    std::uint64_t nextAreaLightId = 1;
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
void registerNativeRigidbody(JSContext *ctx, ScriptHost &host, int ownerId,
                             const std::shared_ptr<Rigidbody> &component);
void registerNativeVehicle(JSContext *ctx, ScriptHost &host, int ownerId,
                           const std::shared_ptr<Vehicle> &component);
void registerNativeFixedJoint(JSContext *ctx, ScriptHost &host, int ownerId,
                              const std::shared_ptr<FixedJoint> &component);
void registerNativeHingeJoint(JSContext *ctx, ScriptHost &host, int ownerId,
                              const std::shared_ptr<HingeJoint> &component);
void registerNativeSpringJoint(JSContext *ctx, ScriptHost &host, int ownerId,
                               const std::shared_ptr<SpringJoint> &component);
void dispatchInteractiveFrame(JSContext *ctx, ScriptHost &host, Window &window,
                              float deltaTime);
void dispatchInteractiveMouseMove(JSContext *ctx, ScriptHost &host,
                                  Window &window, const MousePacket &packet,
                                  float deltaTime);
void dispatchInteractiveMouseScroll(JSContext *ctx, ScriptHost &host,
                                    const MouseScrollPacket &packet,
                                    float deltaTime);

char *normalizeModuleName(JSContext *ctx, const char *baseName,
                          const char *name, void *host);
JSModuleDef *loadModule(JSContext *ctx, const char *name, void *opaque);
bool evalModule(JSContext *ctx, const std::string &name,
                const std::string &src);
JSValue importModuleNamespace(JSContext *ctx, const std::string &name);

ScriptInstance *createScriptInstance(JSContext *ctx,
                                     const std::string &entryModuleName,
                                     const std::string &scriptPath,
                                     const std::string &className);

JSValue jsPrint(JSContext *ctx, JSValueConst this_val, int argc,
                JSValueConst *argv);
} // namespace runtime::scripting

#endif // RUNTIME_SCRIPTING_H
