//
// context.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Context settings for the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/effect.h"
#include "atlas/input.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <json.hpp>
#include <string>
#include <toml.hpp>
#include <vector>

namespace {

#define JSON_READ_BOOL(node, key, target)                                      \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_boolean()) {  \
            (target) = _atlas_json_it->get<bool>();                            \
        }                                                                      \
    } while (0)

#define JSON_READ_FLOAT(node, key, target)                                     \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_number()) {   \
            (target) = _atlas_json_it->get<float>();                           \
        }                                                                      \
    } while (0)

#define JSON_READ_INT(node, key, target)                                       \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_number()) {   \
            (target) = _atlas_json_it->get<int>();                             \
        }                                                                      \
    } while (0)

#define JSON_READ_STRING(node, key, target)                                    \
    do {                                                                       \
        auto _atlas_json_it = (node).find(key);                                \
        if (_atlas_json_it != (node).end() && _atlas_json_it->is_string()) {   \
            (target) = _atlas_json_it->get<std::string>();                     \
        }                                                                      \
    } while (0)

std::string normalizeToken(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '.') {
            continue;
        }
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

std::string resolveRuntimePath(const std::string &baseDir,
                               const std::string &path) {
    const std::filesystem::path candidate(path);
    if (candidate.is_absolute()) {
        return candidate.lexically_normal().string();
    }
    return (std::filesystem::path(baseDir) / candidate)
        .lexically_normal()
        .string();
}

json loadJsonFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }
    json data;
    file >> data;
    return data;
}

bool tryReadVec3(const json &node, const char *key, Position3d &target) {
    auto it = node.find(key);
    if (it == node.end() || !it->is_array() || it->size() != 3) {
        return false;
    }
    target = Position3d((*it)[0].get<float>(), (*it)[1].get<float>(),
                        (*it)[2].get<float>());
    return true;
}

bool tryReadVec2(const json &node, const char *key, Position2d &target) {
    auto it = node.find(key);
    if (it == node.end() || !it->is_array() || it->size() != 2) {
        return false;
    }
    target = Position2d{(*it)[0].get<float>(), (*it)[1].get<float>()};
    return true;
}

Color parseColor(const json &value) {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    if (value.is_array() && (value.size() == 3 || value.size() == 4)) {
        r = value[0].get<float>();
        g = value[1].get<float>();
        b = value[2].get<float>();
        if (value.size() == 4) {
            a = value[3].get<float>();
        }
    } else if (value.is_object()) {
        JSON_READ_FLOAT(value, "r", r);
        JSON_READ_FLOAT(value, "g", g);
        JSON_READ_FLOAT(value, "b", b);
        JSON_READ_FLOAT(value, "a", a);
    } else {
        throw std::runtime_error("Invalid color value");
    }

    const float maxComponent = std::max(std::max(r, g), std::max(b, a));
    if (maxComponent > 1.0f) {
        r /= 255.0f;
        g /= 255.0f;
        b /= 255.0f;
        a /= 255.0f;
    }

    return Color{.r = r, .g = g, .b = b, .a = a};
}

bool tryReadColor(const json &node, const char *key, Color &target) {
    auto it = node.find(key);
    if (it == node.end()) {
        return false;
    }
    target = parseColor(*it);
    return true;
}

Key parseKeyString(const std::string &value) {
    SDL_Scancode scancode = SDL_GetScancodeFromName(value.c_str());
    if (scancode != SDL_SCANCODE_UNKNOWN) {
        return static_cast<Key>(scancode);
    }

    std::string spaced = value;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    std::replace(spaced.begin(), spaced.end(), '-', ' ');
    scancode = SDL_GetScancodeFromName(spaced.c_str());
    if (scancode != SDL_SCANCODE_UNKNOWN) {
        return static_cast<Key>(scancode);
    }

    const std::string token = normalizeToken(value);
    if (token.size() == 1 && std::isalpha(static_cast<unsigned char>(token[0]))) {
        std::string letter(1,
                           static_cast<char>(std::toupper(
                               static_cast<unsigned char>(token[0]))));
        scancode = SDL_GetScancodeFromName(letter.c_str());
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            return static_cast<Key>(scancode);
        }
    }

    if (token.size() == 1 && std::isdigit(static_cast<unsigned char>(token[0]))) {
        std::string digit(1, token[0]);
        scancode = SDL_GetScancodeFromName(digit.c_str());
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            return static_cast<Key>(scancode);
        }
    }

    if (token == "space") {
        return Key::Space;
    }
    if (token == "leftshift" || token == "lshift") {
        return Key::LeftShift;
    }
    if (token == "rightshift" || token == "rshift") {
        return Key::RightShift;
    }
    if (token == "leftcontrol" || token == "leftctrl" || token == "lctrl") {
        return Key::LeftControl;
    }
    if (token == "rightcontrol" || token == "rightctrl" || token == "rctrl") {
        return Key::RightControl;
    }
    if (token == "leftalt" || token == "lalt") {
        return Key::LeftAlt;
    }
    if (token == "rightalt" || token == "ralt") {
        return Key::RightAlt;
    }
    if (token == "leftsuper" || token == "lsuper" || token == "leftgui") {
        return Key::LeftSuper;
    }
    if (token == "rightsuper" || token == "rsuper" || token == "rightgui") {
        return Key::RightSuper;
    }
    if (token == "return") {
        return Key::Enter;
    }

    throw std::runtime_error("Unknown key trigger: " + value);
}

MouseButton parseMouseButtonString(const std::string &value) {
    const std::string token = normalizeToken(value);
    if (token == "mouseleft" || token == "leftmouse" || token == "button1" ||
        token == "mouse1") {
        return MouseButton::Left;
    }
    if (token == "mouseright" || token == "rightmouse" || token == "button2" ||
        token == "mouse2") {
        return MouseButton::Right;
    }
    if (token == "mousemiddle" || token == "middlemouse" ||
        token == "button3" || token == "mouse3") {
        return MouseButton::Middle;
    }
    if (token == "button4" || token == "mouse4") {
        return MouseButton::Button4;
    }
    if (token == "button5" || token == "mouse5") {
        return MouseButton::Button5;
    }
    if (token == "button6" || token == "mouse6") {
        return MouseButton::Button6;
    }
    if (token == "button7" || token == "mouse7") {
        return MouseButton::Button7;
    }
    if (token == "button8" || token == "mouse8") {
        return MouseButton::Button8;
    }
    throw std::runtime_error("Unknown mouse trigger: " + value);
}

Trigger parseTrigger(const json &triggerData) {
    if (triggerData.is_string()) {
        const std::string raw = triggerData.get<std::string>();
        const std::string token = normalizeToken(raw);
        if (token.rfind("mouse", 0) == 0 || token.rfind("button", 0) == 0) {
            return Trigger::fromMouseButton(parseMouseButtonString(raw));
        }
        return Trigger::fromKey(parseKeyString(raw));
    }

    if (!triggerData.is_object()) {
        throw std::runtime_error("Invalid trigger definition");
    }

    std::string type;
    JSON_READ_STRING(triggerData, "type", type);
    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "mouse") {
        std::string buttonName;
        JSON_READ_STRING(triggerData, "button", buttonName);
        if (buttonName.empty()) {
            JSON_READ_STRING(triggerData, "name", buttonName);
        }
        return Trigger::fromMouseButton(parseMouseButtonString(buttonName));
    }

    if (normalizedType == "controller") {
        int controllerId = -1;
        int buttonIndex = -1;
        JSON_READ_INT(triggerData, "id", controllerId);
        JSON_READ_INT(triggerData, "button", buttonIndex);
        JSON_READ_INT(triggerData, "buttonIndex", buttonIndex);
        if (buttonIndex < 0) {
            throw std::runtime_error("Controller trigger is missing button");
        }
        return Trigger::fromControllerButton(controllerId, buttonIndex);
    }

    std::string keyName;
    JSON_READ_STRING(triggerData, "key", keyName);
    if (keyName.empty()) {
        JSON_READ_STRING(triggerData, "name", keyName);
    }
    if (keyName.empty()) {
        throw std::runtime_error("Key trigger is missing key name");
    }
    return Trigger::fromKey(parseKeyString(keyName));
}

AxisTrigger parseAxisTrigger(const json &triggerData) {
    if (triggerData.is_string()) {
        const std::string type = normalizeToken(triggerData.get<std::string>());
        if (type == "mouse") {
            return AxisTrigger::mouse();
        }
        throw std::runtime_error("Unknown axis trigger: " +
                                 triggerData.get<std::string>());
    }

    if (!triggerData.is_object()) {
        throw std::runtime_error("Invalid axis trigger definition");
    }

    std::string type;
    JSON_READ_STRING(triggerData, "type", type);
    const std::string normalizedType = normalizeToken(type);

    if (normalizedType == "mouse") {
        return AxisTrigger::mouse();
    }

    if (normalizedType == "controller") {
        int controllerId = -1;
        int axisIndex = -1;
        int axisIndexY = -1;
        JSON_READ_INT(triggerData, "id", controllerId);
        JSON_READ_INT(triggerData, "index", axisIndex);
        JSON_READ_INT(triggerData, "indexY", axisIndexY);
        if (axisIndex < 0 && triggerData.contains("indexes") &&
            triggerData["indexes"].is_array() &&
            triggerData["indexes"].size() == 2) {
            axisIndex = triggerData["indexes"][0].get<int>();
            axisIndexY = triggerData["indexes"][1].get<int>();
        }
        if (axisIndex < 0) {
            throw std::runtime_error("Controller axis trigger is missing index");
        }
        return AxisTrigger::controller(controllerId, axisIndex,
                                       axisIndexY < 0, axisIndexY);
    }

    if (normalizedType == "custom") {
        if (triggerData.contains("triggers") &&
            triggerData["triggers"].is_array()) {
            const auto &triggers = triggerData["triggers"];
            if (triggers.size() == 2) {
                return AxisTrigger::custom(parseTrigger(triggers[0]),
                                           parseTrigger(triggers[1]), {}, {});
            }
            if (triggers.size() == 4) {
                return AxisTrigger::custom(parseTrigger(triggers[0]),
                                           parseTrigger(triggers[1]),
                                           parseTrigger(triggers[2]),
                                           parseTrigger(triggers[3]));
            }
        }

        Trigger positiveX{};
        Trigger negativeX{};
        Trigger positiveY{};
        Trigger negativeY{};
        bool hasPositive = false;
        bool hasNegative = false;

        auto it = triggerData.find("positiveX");
        if (it != triggerData.end()) {
            positiveX = parseTrigger(*it);
            hasPositive = true;
        }
        it = triggerData.find("negativeX");
        if (it != triggerData.end()) {
            negativeX = parseTrigger(*it);
            hasNegative = true;
        }
        it = triggerData.find("positiveY");
        if (it != triggerData.end()) {
            positiveY = parseTrigger(*it);
        }
        it = triggerData.find("negativeY");
        if (it != triggerData.end()) {
            negativeY = parseTrigger(*it);
        }
        it = triggerData.find("positive");
        if (!hasPositive && it != triggerData.end()) {
            positiveX = parseTrigger(*it);
            hasPositive = true;
        }
        it = triggerData.find("negative");
        if (!hasNegative && it != triggerData.end()) {
            negativeX = parseTrigger(*it);
            hasNegative = true;
        }

        if (!hasPositive || !hasNegative) {
            throw std::runtime_error("Custom axis trigger is incomplete");
        }

        return AxisTrigger::custom(positiveX, negativeX, positiveY, negativeY);
    }

    throw std::runtime_error("Unknown axis trigger type: " + type);
}

std::shared_ptr<InputAction> parseInputAction(const json &actionData) {
    if (!actionData.is_object()) {
        throw std::runtime_error("Input action entry must be an object");
    }

    std::string name;
    JSON_READ_STRING(actionData, "name", name);
    if (name.empty()) {
        throw std::runtime_error("Input action is missing a name");
    }

    std::shared_ptr<InputAction> action;

    if (actionData.contains("triggerAxes") && actionData["triggerAxes"].is_array()) {
        std::vector<AxisTrigger> triggers;
        for (const auto &triggerData : actionData["triggerAxes"]) {
            triggers.push_back(parseAxisTrigger(triggerData));
        }

        action = InputAction::createAxisInputAction(name, triggers);

        JSON_READ_BOOL(actionData, "singleAxis", action->isAxisSingle);
        JSON_READ_BOOL(actionData, "normalize2D", action->normalize2D);
        JSON_READ_BOOL(actionData, "invertControllerY",
                       action->invertControllerY);
        JSON_READ_BOOL(actionData, "clampAxis", action->clampAxis);
        JSON_READ_FLOAT(actionData, "controllerDeadzone",
                        action->controllerDeadzone);
        JSON_READ_FLOAT(actionData, "axisScaleX", action->axisScaleX);
        JSON_READ_FLOAT(actionData, "axisScaleY", action->axisScaleY);

        auto it = actionData.find("axisScale");
        if (it != actionData.end()) {
            if (it->is_number()) {
                action->axisScaleX = it->get<float>();
                action->axisScaleY = it->get<float>();
            } else if (it->is_array() && it->size() == 2) {
                action->axisScaleX = (*it)[0].get<float>();
                action->axisScaleY = (*it)[1].get<float>();
            }
        }

        it = actionData.find("axisClamp");
        if (it != actionData.end() && it->is_array() && it->size() == 2) {
            action->axisClampMin = (*it)[0].get<float>();
            action->axisClampMax = (*it)[1].get<float>();
            action->clampAxis = true;
        }
    } else if (actionData.contains("triggerButtons") &&
               actionData["triggerButtons"].is_array()) {
        std::vector<Trigger> triggers;
        for (const auto &triggerData : actionData["triggerButtons"]) {
            triggers.push_back(parseTrigger(triggerData));
        }
        action = InputAction::createButtonInputAction(name, triggers);
    } else {
        throw std::runtime_error("Input action '" + name +
                                 "' has no triggerButtons or triggerAxes");
    }

    return action;
}

void loadInputActionsFromJson(Window &window, const json &inputData,
                              const std::string &projectDir) {
    if (inputData.is_string()) {
        loadInputActionsFromJson(
            window,
            loadJsonFile(resolveRuntimePath(projectDir,
                                            inputData.get<std::string>())),
            projectDir);
        return;
    }

    const json *actions = nullptr;
    if (inputData.is_array()) {
        actions = &inputData;
    } else if (inputData.is_object()) {
        auto it = inputData.find("actions");
        if (it != inputData.end() && it->is_array()) {
            actions = &(*it);
        } else {
            it = inputData.find("inputActions");
            if (it != inputData.end() && it->is_array()) {
                actions = &(*it);
            }
        }
    }

    if (actions == nullptr) {
        throw std::runtime_error("Invalid input actions payload");
    }

    for (const auto &actionData : *actions) {
        auto action = parseInputAction(actionData);
        if (window.getInputAction(action->name) == nullptr) {
            window.addInputAction(action);
        }
    }
}

std::shared_ptr<Effect> parseEffect(const json &effectData) {
    if (!effectData.is_object()) {
        throw std::runtime_error("Render target effect entry must be an object");
    }

    std::string type;
    JSON_READ_STRING(effectData, "type", type);
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
        float magnitude = 16.0f;
        JSON_READ_FLOAT(effectData, "magnitude", magnitude);
        return Blur::create(magnitude);
    }
    if (normalizedType == "edgedetection") {
        return EdgeDetection::create();
    }
    if (normalizedType == "colorcorrection") {
        ColorCorrectionParameters params;
        JSON_READ_FLOAT(effectData, "exposure", params.exposure);
        JSON_READ_FLOAT(effectData, "contrast", params.contrast);
        JSON_READ_FLOAT(effectData, "saturation", params.saturation);
        JSON_READ_FLOAT(effectData, "gamma", params.gamma);
        JSON_READ_FLOAT(effectData, "temperature", params.temperature);
        JSON_READ_FLOAT(effectData, "tint", params.tint);
        return ColorCorrection::create(params);
    }
    if (normalizedType == "motionblur") {
        MotionBlurParameters params;
        JSON_READ_INT(effectData, "size", params.size);
        JSON_READ_FLOAT(effectData, "separation", params.separation);
        return MotionBlur::create(params);
    }
    if (normalizedType == "chromaticaberration") {
        ChromaticAberrationParameters params;
        JSON_READ_FLOAT(effectData, "red", params.red);
        JSON_READ_FLOAT(effectData, "green", params.green);
        JSON_READ_FLOAT(effectData, "blue", params.blue);
        Position2d direction;
        if (tryReadVec2(effectData, "direction", direction)) {
            params.direction = direction;
        }
        return ChromaticAberration::create(params);
    }
    if (normalizedType == "posterization") {
        PosterizationParameters params;
        JSON_READ_FLOAT(effectData, "levels", params.levels);
        return Posterization::create(params);
    }
    if (normalizedType == "pixelation") {
        PixelationParameters params;
        JSON_READ_INT(effectData, "pixelSize", params.pixelSize);
        return Pixelation::create(params);
    }
    if (normalizedType == "dilation") {
        DilationParameters params;
        JSON_READ_INT(effectData, "size", params.size);
        JSON_READ_FLOAT(effectData, "separation", params.separation);
        return Dilation::create(params);
    }
    if (normalizedType == "filmgrain") {
        FilmGrainParameters params;
        JSON_READ_FLOAT(effectData, "amount", params.amount);
        return FilmGrain::create(params);
    }

    throw std::runtime_error("Unknown render target effect type: " + type);
}

} // namespace

std::shared_ptr<Context> runtime::makeContext(std::string projectFile) {
    auto context = std::make_shared<Context>();

    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);

    int resWidth = 1280;
    int resHeight = 720;
    bool mouseCaptured = false;
    bool multisampling = false;
    float ssaoScale = 0.4f;

    if (auto *windowTable = configTable["window"].as_table()) {
        if (auto *dimensions = (*windowTable)["dimensions"].as_array()) {
            if (dimensions->size() == 2 && (*dimensions)[0].is_integer() &&
                (*dimensions)[1].is_integer()) {
                resWidth = (*dimensions)[0].as_integer()->get();
                resHeight = (*dimensions)[1].as_integer()->get();
            }
        }
        mouseCaptured = (*windowTable)["mouse_capture"].value_or(false);
        multisampling = (*windowTable)["multisampling"].value_or(false);
        ssaoScale = (*windowTable)["ssaoScale"].value_or(0.4f);
    }

    context->window = std::make_unique<Window>(WindowConfiguration{
        .title = "Atlas Runtime",
        .width = resWidth,
        .height = resHeight,
        .renderScale = 1.f,
        .mouseCaptured = mouseCaptured,
        .multisampling = multisampling,
        .ssaoScale = ssaoScale,
    });
    context->projectFile = std::move(projectFile);
    context->projectDir =
        std::filesystem::path(context->projectFile).parent_path().string();
    context->scene = std::make_shared<RuntimeScene>();
    context->scene->context = context;

    return context;
}

void Context::runWindowed() {
    window->setScene(scene.get());
    window->run();
}

void Context::loadProject() {
    if (!std::filesystem::exists(projectFile)) {
        throw std::runtime_error("Project file does not exist: " + projectFile);
    }

    toml::table configTable = toml::parse_file(projectFile);

    std::string defaultRenderer = "normal";
    bool globalIllumination = false;
    std::string mainScene = "main.ascene";
    std::vector<std::string> assetDirectories;
    bool useUpscaling = false;

    if (auto *renderer = configTable["renderer"].as_table()) {
        defaultRenderer = (*renderer)["default"].value_or("normal");
        globalIllumination = (*renderer)["global_illumination"].value_or(false);
        useUpscaling = (*renderer)["use_upscaling"].value_or(false);
    }

    if (auto *gameTable = configTable["game"].as_table()) {
        mainScene = (*gameTable)["main_scene"].value_or("main.ascene");

        assetDirectories.clear();
        if (auto *assets = (*gameTable)["assets"].as_array()) {
            for (const auto &assetDir : *assets) {
                if (assetDir.is_string()) {
                    assetDirectories.push_back(assetDir.as_string()->get());
                }
            }
        }
    }

    config.renderer = defaultRenderer;
    config.globalIllumination = globalIllumination;
    config.mainScene = mainScene;
    config.assetDirectories = assetDirectories;
    config.useUpscaling = useUpscaling;
}

void RuntimeScene::update(Window &window) {
    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving) {
        return;
    }

    if (context->cameraActions.size() >= 3) {
        context->camera->updateWithActions(window, context->cameraActions[0],
                                           context->cameraActions[1],
                                           context->cameraActions[2]);
    } else {
        context->camera->update(window);
    }
}

void RuntimeScene::onMouseMove(Window &window, Movement2d movement) {
    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving || context->cameraActions.size() >= 3) {
        return;
    }
    context->camera->updateLook(window, movement);
}

void RuntimeScene::onMouseScroll(Window &window, Movement2d offset) {
    if (context == nullptr || context->camera == nullptr ||
        !context->cameraAutomaticMoving || context->cameraActions.size() >= 3) {
        return;
    }
    context->camera->updateZoom(window, offset);
}

void Context::loadMainScene(Window &window) {
    RUNTIME_LOG("Loading main scene: " + config.mainScene);
    std::ifstream sceneFile(resolveRuntimePath(projectDir, config.mainScene));
    if (!sceneFile.is_open()) {
        throw std::runtime_error("Failed to open main scene file: " +
                                 config.mainScene);
    }

    json scene;
    sceneFile >> scene;
    loadScene(window, scene);
}

void Context::loadScene(Window &window, const json &sceneData) {
    renderTargets.clear();
    directionalLights.clear();
    pointLights.clear();
    spotlights.clear();
    areaLights.clear();
    cameraActions.clear();
    cameraAutomaticMoving = false;
    camera = std::make_unique<Camera>();
    window.resetInputActions();

    if (sceneData.contains("inputActions")) {
        loadInputActionsFromJson(window, sceneData["inputActions"], projectDir);
    } else if (sceneData.contains("input_actions")) {
        loadInputActionsFromJson(window, sceneData["input_actions"], projectDir);
    }

    if (sceneData.contains("targets") && sceneData["targets"].is_array()) {
        for (const auto &targetData : sceneData["targets"]) {
            if (!targetData.is_object()) {
                throw std::runtime_error("Render target entry must be an object");
            }

            std::string type;
            JSON_READ_STRING(targetData, "type", type);
            if (type.empty()) {
                throw std::runtime_error("Render target is missing a type");
            }

            std::unique_ptr<RenderTarget> target;
            const std::string normalizedType = normalizeToken(type);
            if (normalizedType == "multisampled") {
                target = std::make_unique<RenderTarget>(
                    window, RenderTargetType::Multisampled);
            } else if (normalizedType == "scene") {
                target =
                    std::make_unique<RenderTarget>(window, RenderTargetType::Scene);
            } else {
                throw std::runtime_error("Unknown render target type: " + type);
            }

            if (targetData.contains("effects") && targetData["effects"].is_array()) {
                for (const auto &effectData : targetData["effects"]) {
                    target->addEffect(parseEffect(effectData));
                }
            }

            bool render = false;
            bool display = false;
            JSON_READ_BOOL(targetData, "render", render);
            JSON_READ_BOOL(targetData, "display", display);

            if (render) {
                window.addRenderTarget(target.get());
            }
            if (display) {
                target->display(window);
            }

            std::string name;
            JSON_READ_STRING(targetData, "name", name);
            if (name.empty()) {
                throw std::runtime_error("Render target is missing a name "
                                         "property or it is not a string");
            }
            renderTargets[name] = std::move(target);
        }
    }

    if (sceneData.contains("camera") && sceneData["camera"].is_object()) {
        const auto &cameraData = sceneData["camera"];

        tryReadVec3(cameraData, "position", camera->position);

        Position3d target;
        if (tryReadVec3(cameraData, "target", target)) {
            camera->lookAt(target);
        }

        JSON_READ_FLOAT(cameraData, "fov", camera->fov);
        JSON_READ_FLOAT(cameraData, "nearClip", camera->nearClip);
        JSON_READ_FLOAT(cameraData, "farClip", camera->farClip);
        JSON_READ_FLOAT(cameraData, "orthoSize", camera->orthographicSize);
        JSON_READ_FLOAT(cameraData, "movementSpeed", camera->movementSpeed);
        JSON_READ_FLOAT(cameraData, "mouseSensitivity",
                        camera->mouseSensitivity);
        JSON_READ_FLOAT(cameraData, "controllerLookSensitivity",
                        camera->controllerLookSensitivity);
        JSON_READ_FLOAT(cameraData, "lookSmoothness", camera->lookSmoothness);
        JSON_READ_BOOL(cameraData, "orthographic", camera->useOrthographic);
        JSON_READ_FLOAT(cameraData, "focusDepth", camera->focusDepth);
        JSON_READ_FLOAT(cameraData, "focusRange", camera->focusRange);
        JSON_READ_BOOL(cameraData, "automaticMoving", cameraAutomaticMoving);

        if (cameraData.contains("inputActions")) {
            loadInputActionsFromJson(window, cameraData["inputActions"],
                                     projectDir);
        }

        if (cameraData.contains("actions") && cameraData["actions"].is_array()) {
            for (const auto &actionValue : cameraData["actions"]) {
                if (actionValue.is_string()) {
                    cameraActions.push_back(actionValue.get<std::string>());
                }
            }
        }
    }

    window.setCamera(camera.get());

    if (sceneData.contains("lights") && sceneData["lights"].is_array()) {
        for (const auto &lightData : sceneData["lights"]) {
            if (!lightData.is_object()) {
                throw std::runtime_error("Light entry must be an object");
            }

            std::string type;
            JSON_READ_STRING(lightData, "type", type);
            const std::string normalizedType = normalizeToken(type);

            if (normalizedType == "ambient" || normalizedType == "ambientlight") {
                Color ambientColor = Color::white();
                float intensity = 0.5f;
                tryReadColor(lightData, "color", ambientColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                scene->setAmbientColor(ambientColor);
                scene->setAmbientIntensity(intensity * 4.0f);
                continue;
            }

            if (normalizedType == "directional" ||
                normalizedType == "directionallight") {
                Position3d direction = Position3d::down();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                int shadowResolution = 4096;
                bool castsShadows = false;

                tryReadVec3(lightData, "direction", direction);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);

                auto light = std::make_unique<DirectionalLight>(
                    direction.normalized(), color, shineColor, intensity);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                scene->addDirectionalLight(light.get());
                directionalLights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "point" || normalizedType == "pointlight") {
                Position3d position = Position3d::zero();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                float distance = 50.0f;
                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;

                tryReadVec3(lightData, "position", position);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_FLOAT(lightData, "distance", distance);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                auto light = std::make_unique<Light>(position, color, distance,
                                                     shineColor, intensity);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (addDebugObject) {
                    light->createDebugObject();
                    light->addDebugObject(window);
                }
                scene->addLight(light.get());
                pointLights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "spot" || normalizedType == "spotlight") {
                Position3d position = Position3d::zero();
                Position3d direction = Position3d::down();
                Color color = Color::white();
                Color shineColor = Color::white();
                float intensity = 1.0f;
                float range = 50.0f;
                float cutOff = 35.0f;
                float outerCutoff = 40.0f;
                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;

                tryReadVec3(lightData, "position", position);
                tryReadVec3(lightData, "direction", direction);
                tryReadColor(lightData, "color", color);
                tryReadColor(lightData, "shineColor", shineColor);
                JSON_READ_FLOAT(lightData, "intensity", intensity);
                JSON_READ_FLOAT(lightData, "range", range);
                JSON_READ_FLOAT(lightData, "cutoff", cutOff);
                JSON_READ_FLOAT(lightData, "outerCutoff", outerCutoff);
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                auto light = std::make_unique<Spotlight>(
                    position, direction.normalized(), color, cutOff,
                    outerCutoff, shineColor, intensity, range);
                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (addDebugObject) {
                    light->createDebugObject();
                    light->addDebugObject(window);
                }
                scene->addSpotlight(light.get());
                spotlights.push_back(std::move(light));
                continue;
            }

            if (normalizedType == "area" || normalizedType == "arealight") {
                auto light = std::make_unique<AreaLight>();
                light->right = light->right.normalized();
                light->up = light->up.normalized();

                tryReadVec3(lightData, "position", light->position);
                tryReadVec3(lightData, "right", light->right);
                tryReadVec3(lightData, "up", light->up);
                light->right = light->right.normalized();
                light->up = light->up.normalized();

                Position2d size;
                if (tryReadVec2(lightData, "size", size)) {
                    light->size = Size2d{size.x, size.y};
                }

                tryReadColor(lightData, "color", light->color);
                tryReadColor(lightData, "shineColor", light->shineColor);
                JSON_READ_FLOAT(lightData, "intensity", light->intensity);
                JSON_READ_FLOAT(lightData, "range", light->range);
                JSON_READ_FLOAT(lightData, "angle", light->angle);
                JSON_READ_BOOL(lightData, "castsBothSides",
                               light->castsBothSides);

                int shadowResolution = 2048;
                bool castsShadows = false;
                bool addDebugObject = false;
                JSON_READ_INT(lightData, "shadowResolution", shadowResolution);
                JSON_READ_BOOL(lightData, "castsShadows", castsShadows);
                JSON_READ_BOOL(lightData, "addDebugObject", addDebugObject);

                if (castsShadows) {
                    light->castShadows(window, shadowResolution);
                }
                if (addDebugObject) {
                    light->createDebugObject();
                    light->addDebugObject(window);
                }
                scene->addAreaLight(light.get());
                areaLights.push_back(std::move(light));
                continue;
            }

            throw std::runtime_error("Unknown light type: " + type);
        }
    }
}
