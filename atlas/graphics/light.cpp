/*
 light.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Lighting helper functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/light.h"
#include "atlas/core/shader.h"
#include "atlas/object.h"
#include "atlas/texture.h"
#include "atlas/tracer/log.h"
#include "atlas/window.h"
#include <tuple>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Light::createDebugObject() {
    CoreObject sphere = createSphere(0.05f, 36, 18, this->color);
    sphere.setPosition(this->position);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    sphere.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(sphere);
}

void Light::setColor(Color color) {
    this->color = color;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(color);
    }
}

void Light::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}

PointLightConstants Light::calculateConstants() const {
    struct Entry {
        float distance, constant, linear, quadratic;
    };
    static const Entry table[] = {
        {.distance = 7, .constant = 1.0f, .linear = 0.7f, .quadratic = 1.8f},
        {.distance = 13, .constant = 1.0f, .linear = 0.35f, .quadratic = 0.44f},
        {.distance = 20, .constant = 1.0f, .linear = 0.22f, .quadratic = 0.20f},
        {.distance = 32, .constant = 1.0f, .linear = 0.14f, .quadratic = 0.07f},
        {.distance = 50,
         .constant = 1.0f,
         .linear = 0.09f,
         .quadratic = 0.032f},
        {.distance = 65,
         .constant = 1.0f,
         .linear = 0.07f,
         .quadratic = 0.017f},
        {.distance = 100,
         .constant = 1.0f,
         .linear = 0.045f,
         .quadratic = 0.0075f},
        {.distance = 160,
         .constant = 1.0f,
         .linear = 0.027f,
         .quadratic = 0.0028f},
        {.distance = 200,
         .constant = 1.0f,
         .linear = 0.022f,
         .quadratic = 0.0019f},
        {.distance = 325,
         .constant = 1.0f,
         .linear = 0.014f,
         .quadratic = 0.0007f},
        {.distance = 600,
         .constant = 1.0f,
         .linear = 0.007f,
         .quadratic = 0.0002f},
        {.distance = 3250,
         .constant = 1.0f,
         .linear = 0.0014f,
         .quadratic = 0.000007f},
    };

    const int n = sizeof(table) / sizeof(table[0]);

    if (distance <= table[0].distance) {
        return {.distance = distance,
                .constant = table[0].constant,
                .linear = table[0].linear,
                .quadratic = table[0].quadratic,
                .radius = 0.0f};
    }
    if (distance >= table[n - 1].distance) {
        return {.distance = distance,
                .constant = table[n - 1].constant,
                .linear = table[n - 1].linear,
                .quadratic = table[n - 1].quadratic,
                .radius = 0.0f};
    }

    for (int i = 0; i < n - 1; i++) {
        if (distance >= table[i].distance &&
            distance <= table[i + 1].distance) {
            float t = (distance - table[i].distance) /
                      (table[i + 1].distance - table[i].distance);
            float constant = table[i].constant +
                             (t * (table[i + 1].constant - table[i].constant));
            float linear =
                table[i].linear + (t * (table[i + 1].linear - table[i].linear));
            float quadratic =
                table[i].quadratic +
                (t * (table[i + 1].quadratic - table[i].quadratic));
            float radius =
                (-linear + sqrt((linear * linear) -
                                ((constant - (256.0f / 5.0f) * distance) * 4 *
                                 quadratic))) /
                (2 * quadratic);
            return {.distance = distance,
                    .constant = constant,
                    .linear = linear,
                    .quadratic = quadratic,
                    .radius = radius};
        }
    }

    return {.distance = distance,
            .constant = 1.0f,
            .linear = 0.0f,
            .quadratic = 0.0f,
            .radius = 0.0f};
}

void Spotlight::createDebugObject() {
    CoreObject pyramid = createPyramid({0.1f, 0.1f, 0.1f}, this->color);
    pyramid.setPosition(this->position);
    pyramid.lookAt(this->position + this->direction);
    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);

    pyramid.createAndAttachProgram(vShader, shader);
    this->debugObject = std::make_shared<CoreObject>(pyramid);
}

void Spotlight::setColor(Color color) {
    this->color = color;
    if (this->debugObject != nullptr) {
        this->debugObject->setColor(color);
    }
}

void Spotlight::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}

void Spotlight::updateDebugObjectRotation() {
    if (this->debugObject != nullptr) {
        this->debugObject->setPosition(this->position);
        this->debugObject->lookAt(this->position + this->direction);
    }
}

void Spotlight::lookAt(const Position3d &target) {
    Magnitude3d newDirection = {target.x - this->position.x,
                                target.y - this->position.y,
                                target.z - this->position.z};

    this->direction = newDirection.normalized();

    updateDebugObjectRotation();
}

void Spotlight::castShadows(Window &window, int resolution) {
    atlas_log("Enabling shadow casting for spotlight (resolution: " +
              std::to_string(resolution) + ")");
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::Shadow, resolution);
    }
    this->doesCastShadows = true;
}

void DirectionalLight::castShadows(Window &window, int resolution) {
    atlas_log("Enabling shadow casting for directional light (resolution: " +
              std::to_string(resolution) + ")");
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::Shadow, resolution);
    }
    this->doesCastShadows = true;
}

ShadowParams DirectionalLight::calculateLightSpaceMatrix(
    const std::vector<Renderable *> objects) const {

    if (objects.empty()) {
        glm::mat4 identity = glm::mat4(1.0f);
        return {identity, identity, 0.0f, 0.0f};
    }

    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

    for (auto *obj : objects) {
        if (!obj->canCastShadows())
            continue;
        glm::vec3 pos = obj->getPosition().toGlm();
        glm::vec3 scale = obj->getScale().toGlm();

        const auto &vertices = obj->getVertices();
        if (vertices.empty())
            continue;

        glm::vec3 localMin(std::numeric_limits<float>::max());
        glm::vec3 localMax(std::numeric_limits<float>::lowest());

        for (const auto &vertex : vertices) {
            glm::vec3 localPos = vertex.position.toGlm();
            localMin = glm::min(localMin, localPos);
            localMax = glm::max(localMax, localPos);
        }

        glm::vec3 scaledMin = localMin * scale;
        glm::vec3 scaledMax = localMax * scale;
        glm::vec3 worldMin = pos + scaledMin;
        glm::vec3 worldMax = pos + scaledMax;

        minPos = glm::min(minPos, worldMin);
        maxPos = glm::max(maxPos, worldMax);
    }

    glm::vec3 padding(5.0f);
    minPos -= padding;
    maxPos += padding;

    glm::vec3 center = (minPos + maxPos) * 0.5f;
    glm::vec3 extent = maxPos - minPos;

    glm::vec3 lightDir = glm::normalize(direction.toGlm());

    float sceneRadius = glm::length(extent) * 0.5f;
    float lightDistance = sceneRadius + 50.0f;
    glm::vec3 lightPos = center - lightDir * lightDistance;

    glm::vec3 up =
        glm::abs(lightDir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::mat4 lightView = glm::lookAt(lightPos, center, up);

    std::vector<glm::vec3> corners = {glm::vec3(minPos.x, minPos.y, minPos.z),
                                      glm::vec3(maxPos.x, minPos.y, minPos.z),
                                      glm::vec3(minPos.x, maxPos.y, minPos.z),
                                      glm::vec3(maxPos.x, maxPos.y, minPos.z),
                                      glm::vec3(minPos.x, minPos.y, maxPos.z),
                                      glm::vec3(maxPos.x, minPos.y, maxPos.z),
                                      glm::vec3(minPos.x, maxPos.y, maxPos.z),
                                      glm::vec3(maxPos.x, maxPos.y, maxPos.z)};

    glm::vec3 lightSpaceMin(std::numeric_limits<float>::max());
    glm::vec3 lightSpaceMax(std::numeric_limits<float>::lowest());

    for (const auto &corner : corners) {
        glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
        lightSpaceMin = glm::min(lightSpaceMin, glm::vec3(lightSpaceCorner));
        lightSpaceMax = glm::max(lightSpaceMax, glm::vec3(lightSpaceCorner));
    }

    float left = lightSpaceMin.x;
    float right = lightSpaceMax.x;
    float bottom = lightSpaceMin.y;
    float top = lightSpaceMax.y;
    float near_plane = -lightSpaceMax.z - 10.0f;
    float far_plane = -lightSpaceMin.z + 10.0f;

    if (near_plane >= far_plane) {
        near_plane = 0.1f;
        far_plane = lightDistance * 2.0f;
    }

    glm::mat4 lightProjection =
        glm::ortho(left, right, bottom, top, near_plane, far_plane);

    float bias = 0.0002f * glm::length(extent);

    return {.lightView = lightView,
            .lightProjection = lightProjection,
            .bias = bias,
            .farPlane = 0.0f};
}

std::tuple<glm::mat4, glm::mat4> Spotlight::calculateLightSpaceMatrix() const {
    float near_plane = 0.1f, far_plane = 100.f;
    glm::mat4 lightProjection =
        glm::perspective(outerCutoff * 2.0f, 1.0f, near_plane, far_plane);
    glm::vec3 lightDir = glm::normalize(direction.toGlm());
    glm::vec3 lightPos = position.toGlm();
    glm::mat4 lightView =
        glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    return {lightView, lightProjection};
}

void Light::castShadows(Window &window, int resolution) {
    if (this->shadowRenderTarget == nullptr) {
        this->shadowRenderTarget =
            new RenderTarget(window, RenderTargetType::CubeShadow, resolution);
    }
    this->doesCastShadows = true;
}

std::vector<glm::mat4> Light::calculateShadowTransforms() {
    float aspect = (float)shadowRenderTarget->texture.creationData.width /
                   (float)shadowRenderTarget->texture.creationData.height;
    float near = 0.1f;
    float far = this->distance;
    glm::mat4 shadowProj =
        glm::perspective(glm::radians(90.0f), aspect, near, far);
    std::vector<glm::mat4> shadowTransforms;
    glm::vec3 lightPos = this->position.toGlm();
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0),
                                 glm::vec3(0.0, 0.0, 1.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0),
                                 glm::vec3(0.0, 0.0, -1.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0),
                                 glm::vec3(0.0, -1.0, 0.0)));
    return shadowTransforms;
}

void AreaLight::createDebugObject() {
    double w = this->size.width * 0.5;
    double h = this->size.height * 0.5;
    Color emissiveColor = this->color * 2.5f;
    emissiveColor.a = this->color.a;

    std::vector<CoreVertex> vertices = {
        {{-w, -h, 0.0},
         emissiveColor,
         {0.0, 0.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{w, -h, 0.0},
         emissiveColor,
         {1.0, 0.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{w, h, 0.0},
         emissiveColor,
         {1.0, 1.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
        {{-w, h, 0.0},
         emissiveColor,
         {0.0, 1.0},
         {0.0f, 0.0f, 1.0f},
         {1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f}},
    };

    std::vector<Index> indices = {
        0,
        1,
        2,
        2,
        3,
        0,
        0,
        3,
        2,
        2,
        1,
        0,
    };

    CoreObject plane;
    plane.attachVertices(vertices);
    plane.attachIndices(indices);

    glm::vec3 desiredRight = glm::normalize(this->right.toGlm());
    glm::vec3 desiredUp = glm::normalize(this->up.toGlm());
    glm::vec3 desiredNormal =
        glm::normalize(glm::cross(desiredRight, desiredUp));

    plane.setPosition(this->position);

    glm::vec3 center = this->position.toGlm();
    plane.lookAt(Position3d::fromGlm(center + desiredNormal),
                 Position3d::fromGlm(desiredUp));

    FragmentShader shader =
        FragmentShader::fromDefaultShader(AtlasFragmentShader::Color);
    VertexShader vShader =
        VertexShader::fromDefaultShader(AtlasVertexShader::Color);
    plane.createAndAttachProgram(vShader, shader);
    plane.useDeferredRendering = false;

    this->debugObject = std::make_shared<CoreObject>(plane);
}

void AreaLight::addDebugObject(Window &window) {
    if (this->debugObject == nullptr) {
        this->createDebugObject();
    }
    window.addObject(this->debugObject.get());
}
