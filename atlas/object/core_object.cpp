/*
 core_object.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Core Object implementation and logic
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/window.h"
#include "opal/opal.h"
#include <algorithm>
#include <glad/glad.h>
#include <limits>
#include <random>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

std::vector<opal::VertexAttributeBinding>
makeInstanceAttributeBindings(const std::shared_ptr<opal::Buffer> &buffer) {
    std::vector<opal::VertexAttributeBinding> bindings;
    bindings.reserve(4);
    std::size_t vec4Size = sizeof(glm::vec4);
    for (unsigned int i = 0; i < 4; ++i) {
        opal::VertexAttribute attribute{"instanceModel" + std::to_string(i),
                                        opal::VertexAttributeType::Float,
                                        static_cast<uint>(i * vec4Size),
                                        static_cast<uint>(6 + i),
                                        false,
                                        4,
                                        static_cast<uint>(sizeof(glm::mat4)),
                                        opal::VertexBindingInputRate::Instance,
                                        1};
        bindings.push_back({attribute, buffer});
    }
    return bindings;
}

} // namespace

std::vector<LayoutDescriptor> CoreVertex::getLayoutDescriptors() {
    return {{"position", 0, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, position)},
            {"color", 1, 4, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, color)},
            {"textureCoordinates", 2, 2, opal::VertexAttributeType::Float,
             false, sizeof(CoreVertex),
             offsetof(CoreVertex, textureCoordinate)},
            {"normal", 3, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, normal)},
            {"tangent", 4, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, tangent)},
            {"bitangent", 5, 3, opal::VertexAttributeType::Float, false,
             sizeof(CoreVertex), offsetof(CoreVertex, bitangent)}};
}

CoreObject::CoreObject() : vao(nullptr), vbo(nullptr), ebo(nullptr) {
    shaderProgram = ShaderProgram::defaultProgram();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, UINT32_MAX);

    id = dist(gen);
}

void CoreObject::attachProgram(const ShaderProgram &program) {
    shaderProgram = program;
    if (shaderProgram.programId == 0) {
        shaderProgram.compile();
    }
    this->refreshPipeline();
}

void CoreObject::createAndAttachProgram(VertexShader &vertexShader,
                                        FragmentShader &fragmentShader) {
    if (vertexShader.shaderId == 0) {
        vertexShader.compile();
    }

    if (fragmentShader.shaderId == 0) {
        fragmentShader.compile();
    }

    shaderProgram = ShaderProgram(vertexShader, fragmentShader);
    shaderProgram.compile();
    this->refreshPipeline();
}

void CoreObject::renderColorWithTexture() {
    useColor = true;
    useTexture = true;
}

void CoreObject::renderOnlyColor() {
    useColor = true;
    useTexture = false;
}

void CoreObject::renderOnlyTexture() {
    useColor = false;
    useTexture = true;
}

void CoreObject::attachTexture(const Texture &tex) {
    textures.push_back(tex);
    useTexture = true;
    useColor = false;
}

void CoreObject::setColor(const Color &color) {
    for (auto &vertex : vertices) {
        vertex.color = color;
    }
    useColor = true;
    useTexture = false;
}

void CoreObject::attachVertices(const std::vector<CoreVertex> &newVertices) {
    if (newVertices.empty()) {
        throw std::runtime_error("Cannot attach empty vertex array");
    }

    vertices = newVertices;
}

void CoreObject::attachIndices(const std::vector<Index> &newIndices) {
    indices = newIndices;
}

void CoreObject::setPosition(const Position3d &newPosition) {
    Position3d oldPosition = position;
    Position3d delta = newPosition - oldPosition;
    position = newPosition;

    if (hasPhysics && body != nullptr) {
        body->position = position;
    }

    if (!instances.empty()) {
        for (auto &instance : instances) {
            instance.position += delta;
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::setRotation(const Rotation3d &newRotation) {
    Rotation3d oldRotation = rotation;
    Rotation3d delta = newRotation - oldRotation;
    rotation = newRotation;

    if (hasPhysics && body != nullptr) {
        body->orientation = rotation.toGlmQuat();
    }

    if (!instances.empty()) {
        for (auto &instance : instances) {
            instance.rotation = instance.rotation + delta;
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::setScale(const Scale3d &newScale) {
    Scale3d oldScale = scale;
    scale = newScale;

    if (!instances.empty()) {
        const auto computeFactor = [](double newValue, double oldValue) {
            const double epsilon = std::numeric_limits<double>::epsilon();
            if (std::abs(oldValue) <= epsilon) {
                return newValue;
            }
            return newValue / oldValue;
        };

        double factorX = computeFactor(newScale.x, oldScale.x);
        double factorY = computeFactor(newScale.y, oldScale.y);
        double factorZ = computeFactor(newScale.z, oldScale.z);

        for (auto &instance : instances) {
            instance.scale = {instance.scale.x * factorX,
                              instance.scale.y * factorY,
                              instance.scale.z * factorZ};
            instance.updateModelMatrix();
        }
    }

    updateModelMatrix();
}

void CoreObject::move(const Position3d &delta) {
    setPosition(position + delta);
}

void CoreObject::rotate(const Rotation3d &delta) {
    setRotation(rotation + delta);
}

void CoreObject::lookAt(const Position3d &target, const Normal3d &up) {
    glm::vec3 pos = position.toGlm();
    glm::vec3 targetPos = target.toGlm();
    glm::vec3 upVec = up.toGlm();

    glm::vec3 forward = glm::normalize(targetPos - pos);

    glm::vec3 right = glm::normalize(glm::cross(forward, upVec));

    glm::vec3 realUp = glm::cross(right, forward);

    glm::mat3 rotMatrix;
    rotMatrix[0] = right;    // X-axis
    rotMatrix[1] = realUp;   // Y-axis
    rotMatrix[2] = -forward; // Z-axis (negative because OpenGL looks down -Z)

    float pitch, yaw, roll;

    pitch = glm::degrees(asin(glm::clamp(rotMatrix[2][1], -1.0f, 1.0f)));

    if (abs(cos(glm::radians(pitch))) > 0.00001f) {
        yaw = glm::degrees(atan2(-rotMatrix[2][0], rotMatrix[2][2]));
        roll = glm::degrees(atan2(-rotMatrix[0][1], rotMatrix[1][1]));
    } else {
        yaw = glm::degrees(atan2(rotMatrix[1][0], rotMatrix[0][0]));
        roll = 0.0f;
    }

    rotation = Rotation3d{roll, yaw, pitch};
    updateModelMatrix();
}

void CoreObject::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.roll)),
                    glm::vec3(0, 0, 1));
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.pitch)),
                    glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(
        rotation_matrix, glm::radians(float(rotation.yaw)), glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    model = translation_matrix * rotation_matrix * scale_matrix;
}

void CoreObject::initialize() {
    for (auto &component : components) {
        component->init();
    }
    if (vertices.empty()) {
        throw std::runtime_error("No vertices attached to the object");
    }

    if (vao == nullptr) {
        vao = opal::DrawingState::create(nullptr);
    }

    vbo = opal::Buffer::create(opal::BufferUsage::VertexBuffer,
                               vertices.size() * sizeof(CoreVertex),
                               vertices.data());

    if (!indices.empty()) {
        ebo = opal::Buffer::create(opal::BufferUsage::IndexArray,
                                   indices.size() * sizeof(Index),
                                   indices.data());
    }

    vao->setBuffers(vbo, ebo);

    this->pipeline = opal::Pipeline::create();

    std::vector<LayoutDescriptor> layoutDescriptors =
        CoreVertex::getLayoutDescriptors();

    std::vector<opal::VertexAttribute> vertexAttributes;
    opal::VertexBinding vertexBinding;

    for (const auto &attr : layoutDescriptors) {
        vertexAttributes.push_back(opal::VertexAttribute{
            attr.name, attr.type, static_cast<uint>(attr.offset),
            static_cast<uint>(attr.layoutPos), attr.normalized,
            static_cast<uint>(attr.size), static_cast<uint>(attr.stride),
            opal::VertexBindingInputRate::Vertex, 0});
    }

    vertexBinding = opal::VertexBinding{(uint)layoutDescriptors[0].stride,
                                        opal::VertexBindingInputRate::Vertex};

    std::vector<opal::VertexAttributeBinding> attributeBindings;
    attributeBindings.reserve(vertexAttributes.size());
    for (const auto &attribute : vertexAttributes) {
        attributeBindings.push_back({attribute, vbo});
    }
    vao->configureAttributes(attributeBindings);

    if (!instances.empty()) {
        std::vector<glm::mat4> modelMatrices;
        for (auto &instance : instances) {
            instance.updateModelMatrix();
            modelMatrices.push_back(instance.getModelMatrix());
        }

        instanceVBO = opal::Buffer::create(opal::BufferUsage::GeneralPurpose,
                                           instances.size() * sizeof(glm::mat4),
                                           modelMatrices.data());
        auto instanceBindings = makeInstanceAttributeBindings(instanceVBO);
        vao->configureAttributes(instanceBindings);
    }

    this->pipeline->setVertexAttributes(vertexAttributes, vertexBinding);

    vao->unbind();
}

std::optional<std::shared_ptr<opal::Pipeline>> CoreObject::getPipeline() {
    if (this->pipeline == nullptr) {
        return std::nullopt;
    }
    return this->pipeline;
}

void CoreObject::setPipeline(std::shared_ptr<opal::Pipeline> &newPipeline) {
    this->pipeline = newPipeline;
}

void CoreObject::refreshPipeline() {
    if (Window::mainWindow == nullptr) {
        return;
    }

    if (this->pipeline == nullptr) {
        this->pipeline = opal::Pipeline::create();
    }

    Window &window = *Window::mainWindow;

    int viewportWidth = window.viewportWidth;
    int viewportHeight = window.viewportHeight;
    if (viewportWidth == 0 || viewportHeight == 0) {
        Size2d size = window.getSize();
        viewportWidth = static_cast<int>(size.width);
        viewportHeight = static_cast<int>(size.height);
    }

    this->pipeline->setViewport(window.viewportX, window.viewportY,
                                viewportWidth, viewportHeight);
    this->pipeline->setPrimitiveStyle(window.primitiveStyle);
    this->pipeline->setRasterizerMode(window.rasterizerMode);
    this->pipeline->setCullMode(window.cullMode);
    this->pipeline->setFrontFace(window.frontFace);
    this->pipeline->enableDepthTest(window.useDepth);
    this->pipeline->setDepthCompareOp(window.depthCompareOp);
    this->pipeline->enableBlending(window.useBlending);
    this->pipeline->setBlendFunc(window.srcBlend, window.dstBlend);

    this->pipeline = this->shaderProgram.requestPipeline(this->pipeline);
}

void CoreObject::render(float dt, bool updatePipeline) {
    for (auto &component : components) {
        component->update(dt);
    }
    if (!isVisible) {
        return;
    }
    if (shaderProgram.programId == 0) {
        throw std::runtime_error("Shader program not compiled");
    }

    if (updatePipeline || this->pipeline == nullptr) {
        this->refreshPipeline();
    }

    if (this->pipeline != nullptr) {
        this->pipeline->bind();
    } else {
        int currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (static_cast<Id>(currentProgram) != shaderProgram.programId) {
            glUseProgram(shaderProgram.programId);
        }
    }

    shaderProgram.setUniformMat4f("model", model);
    shaderProgram.setUniformMat4f("view", view);
    shaderProgram.setUniformMat4f("projection", projection);

    shaderProgram.setUniform1i("useColor", useColor ? 1 : 0);
    shaderProgram.setUniform1i("useTexture", useTexture ? 1 : 0);

    int boundTextures = 0;
    int boundCubemaps = 0;

    const bool shaderSupportsTextures =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Textures) !=
        shaderProgram.capabilities.end();

    if (shaderSupportsTextures) {
        shaderProgram.setUniform1i("textureCount", 0);
        shaderProgram.setUniform1i("cubeMapCount", 0);
    }

    if (!textures.empty() && useTexture && shaderSupportsTextures) {
        int count = std::min((int)textures.size(), 10);
        shaderProgram.setUniform1i("textureCount", count);
        GLint units[10];
        for (int i = 0; i < count; i++)
            units[i] = i;

        for (int i = 0; i < count; i++) {
            std::string uniformName = "texture" + std::to_string(i + 1) + "";
            shaderProgram.setUniform1i(uniformName, i);
        }

        GLint cubeMapUnits[5];
        for (int i = 0; i < 5; i++)
            cubeMapUnits[i] = 10 + i;
        shaderProgram.setUniform1i("cubeMapCount", 5);
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1) + "";
            shaderProgram.setUniform1i(uniformName, i + 10);
        }

        GLint textureTypes[10];
        for (int i = 0; i < count; i++)
            textureTypes[i] = static_cast<int>(textures[i].type);
        for (int i = 0; i < count; i++) {
            std::string uniformName = "textureTypes[" + std::to_string(i) + "]";
            shaderProgram.setUniform1i(uniformName, textureTypes[i]);
        }

        for (int i = 0; i < count; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
            boundTextures++;
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Material) !=
        shaderProgram.capabilities.end()) {
        // Set material properties
        shaderProgram.setUniform3f("material.albedo", material.albedo.r,
                                   material.albedo.g, material.albedo.b);
        shaderProgram.setUniform1f("material.metallic", material.metallic);
        shaderProgram.setUniform1f("material.roughness", material.roughness);
        shaderProgram.setUniform1f("material.ao", material.ao);
    }

    const bool shaderSupportsIbl =
        std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::IBL) != shaderProgram.capabilities.end();

    bool hasHdrEnvironment = std::any_of(
        textures.begin(), textures.end(), [](const Texture &texture) {
            return texture.type == TextureType::HDR;
        });

    const bool useIbl = shaderSupportsIbl && hasHdrEnvironment;
    shaderProgram.setUniformBool("useIBL", useIbl);

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Lighting) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();

        // Set ambient light
        Color ambientColor = scene->getAmbientColor();
        float ambientIntensity = scene->getAmbientIntensity();
        if (!useIbl && scene->isAutomaticAmbientEnabled()) {
            ambientColor = scene->getAutomaticAmbientColor();
            ambientIntensity = scene->getAutomaticAmbientIntensity();
        }
        shaderProgram.setUniform4f("ambientLight.color",
                                   static_cast<float>(ambientColor.r),
                                   static_cast<float>(ambientColor.g),
                                   static_cast<float>(ambientColor.b), 1.0f);
        shaderProgram.setUniform1f("ambientLight.intensity",
                                   ambientIntensity / 2.0f);

        // Set camera position
        shaderProgram.setUniform3f(
            "cameraPosition", window->getCamera()->position.x,
            window->getCamera()->position.y, window->getCamera()->position.z);

        // Send directional lights
        int dirLightCount = std::min((int)scene->directionalLights.size(), 256);
        shaderProgram.setUniform1i("directionalLightCount", dirLightCount);

        for (int i = 0; i < dirLightCount; i++) {
            DirectionalLight *light = scene->directionalLights[i];
            std::string baseName =
                "directionalLights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".direction",
                                       light->direction.x, light->direction.y,
                                       light->direction.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);
        }

        // Send point lights

        int pointLightCount = std::min((int)scene->pointLights.size(), 256);
        shaderProgram.setUniform1i("pointLightCount", pointLightCount);

        for (int i = 0; i < pointLightCount; i++) {
            Light *light = scene->pointLights[i];
            std::string baseName = "pointLights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);

            PointLightConstants plc = light->calculateConstants();
            shaderProgram.setUniform1f(baseName + ".constant", plc.constant);
            shaderProgram.setUniform1f(baseName + ".linear", plc.linear);
            shaderProgram.setUniform1f(baseName + ".quadratic", plc.quadratic);
            shaderProgram.setUniform1f(baseName + ".radius", plc.radius);
        }

        // Send spotlights

        int spotlightCount = std::min((int)scene->spotlights.size(), 256);
        shaderProgram.setUniform1i("spotlightCount", spotlightCount);

        for (int i = 0; i < spotlightCount; i++) {
            Spotlight *light = scene->spotlights[i];
            std::string baseName = "spotlights[" + std::to_string(i) + "]";
            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform3f(baseName + ".direction",
                                       light->direction.x, light->direction.y,
                                       light->direction.z);
            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);
            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);
            shaderProgram.setUniform1f(baseName + ".cutOff", light->cutOff);
            shaderProgram.setUniform1f(baseName + ".outerCutOff",
                                       light->outerCutoff);
        }

        // Send area lights

        int areaLightCount = std::min((int)scene->areaLights.size(), 256);
        shaderProgram.setUniform1i("areaLightCount", areaLightCount);

        for (int i = 0; i < areaLightCount; i++) {
            AreaLight *light = scene->areaLights[i];
            std::string baseName = "areaLights[" + std::to_string(i) + "]";

            shaderProgram.setUniform3f(baseName + ".position",
                                       light->position.x, light->position.y,
                                       light->position.z);

            shaderProgram.setUniform3f(baseName + ".right", light->right.x,
                                       light->right.y, light->right.z);

            shaderProgram.setUniform3f(baseName + ".up", light->up.x,
                                       light->up.y, light->up.z);

            shaderProgram.setUniform2f(baseName + ".size",
                                       static_cast<float>(light->size.width),
                                       static_cast<float>(light->size.height));

            shaderProgram.setUniform3f(baseName + ".diffuse", light->color.r,
                                       light->color.g, light->color.b);

            shaderProgram.setUniform3f(baseName + ".specular",
                                       light->shineColor.r, light->shineColor.g,
                                       light->shineColor.b);

            shaderProgram.setUniform1f(baseName + ".angle", light->angle);
            shaderProgram.setUniform1i(baseName + ".castsBothSides",
                                       light->castsBothSides ? 1 : 0);
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::LightDeferred) !=
        shaderProgram.capabilities.end()) {
        // Bind G-Buffer textures
        Window *window = Window::mainWindow;
        RenderTarget *gBuffer = window->gBuffer.get();
        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gPosition.id);
        shaderProgram.setUniform1i("gPosition", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gNormal.id);
        shaderProgram.setUniform1i("gNormal", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gAlbedoSpec.id);
        shaderProgram.setUniform1i("gAlbedoSpec", boundTextures);
        boundTextures++;

        glActiveTexture(GL_TEXTURE0 + boundTextures);
        glBindTexture(GL_TEXTURE_2D, gBuffer->gMaterial.id);
        shaderProgram.setUniform1i("gMaterial", boundTextures);
        boundTextures++;
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Shadows) !=
        shaderProgram.capabilities.end()) {
        for (int i = 0; i < 5; i++) {
            std::string uniformName = "cubeMap" + std::to_string(i + 1);
            shaderProgram.setUniform1i(uniformName, i + 10);
        }
        // Bind shadow maps
        Scene *scene = Window::mainWindow->currentScene;

        int boundParameters = 0;

        // Cycle though directional lights
        for (auto light : scene->directionalLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundTextures);
            ShadowParams shadowParams = light->calculateLightSpaceMatrix(
                Window::mainWindow->renderables);
            shaderProgram.setUniformMat4f(baseName + ".lightView",
                                          shadowParams.lightView);
            shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                          shadowParams.lightProjection);
            shaderProgram.setUniform1f(baseName + ".bias", shadowParams.bias);
            shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

            boundParameters++;
            boundTextures++;
        }

        // Cycle though spotlights
        for (auto light : scene->spotlights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_2D, light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundTextures);
            std::tuple<glm::mat4, glm::mat4> lightSpace =
                light->calculateLightSpaceMatrix();
            shaderProgram.setUniformMat4f(baseName + ".lightView",
                                          std::get<0>(lightSpace));
            shaderProgram.setUniformMat4f(baseName + ".lightProjection",
                                          std::get<1>(lightSpace));
            shaderProgram.setUniform1f(baseName + ".bias", 0.005f);
            shaderProgram.setUniform1f(baseName + ".isPointLight", 0);

            boundParameters++;
            boundTextures++;
        }

        for (auto light : scene->pointLights) {
            if (!light->doesCastShadows) {
                continue;
            }
            if (boundTextures + 6 >= 16) {
                break;
            }

            glActiveTexture(GL_TEXTURE0 + 10 + boundCubemaps);

            glBindTexture(GL_TEXTURE_CUBE_MAP,
                          light->shadowRenderTarget->texture.id);
            std::string baseName =
                "shadowParams[" + std::to_string(boundParameters) + "]";
            shaderProgram.setUniform1i(baseName + ".textureIndex",
                                       boundCubemaps);
            shaderProgram.setUniform1f(baseName + ".farPlane", light->distance);
            shaderProgram.setUniform3f(baseName + ".lightPos",
                                       light->position.x, light->position.y,
                                       light->position.z);
            shaderProgram.setUniform1i(baseName + ".isPointLight", 1);

            boundParameters++;
            boundTextures += 6;
        }

        shaderProgram.setUniform1i("shadowParamCount", boundParameters);

        GLint units[16];
        for (int i = 0; i < boundTextures; i++)
            units[i] = i;

        glUniform1iv(glGetUniformLocation(shaderProgram.programId, "textures"),
                     boundTextures, units);
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::EnvironmentMapping) !=
        shaderProgram.capabilities.end()) {
        // Bind skybox
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        if (scene->skybox != nullptr) {
            glActiveTexture(GL_TEXTURE0 + boundTextures);
            glBindTexture(GL_TEXTURE_CUBE_MAP, scene->skybox->cubemap.id);
            shaderProgram.setUniform1i("skybox", boundTextures);
            boundTextures++;
        }
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Environment) !=
        shaderProgram.capabilities.end()) {
        Window *window = Window::mainWindow;
        Scene *scene = window->getCurrentScene();
        shaderProgram.setUniform1f("environment.rimLightIntensity",
                                   scene->environment.rimLight.intensity);
        shaderProgram.setUniform3f("environment.rimLightColor",
                                   scene->environment.rimLight.color.r,
                                   scene->environment.rimLight.color.g,
                                   scene->environment.rimLight.color.b);
    }

    if (std::find(shaderProgram.capabilities.begin(),
                  shaderProgram.capabilities.end(),
                  ShaderCapability::Instances) !=
            shaderProgram.capabilities.end() &&
        !instances.empty()) {
        if (this->instances != this->savedInstances) {
            updateInstances();
            this->savedInstances = this->instances;
        }
        // Update instance buffer data
        vao->bind();
        shaderProgram.setUniform1i("isInstanced", 1);

        if (!indices.empty()) {
            glDrawElementsInstanced(GL_TRIANGLES, indices.size(),
                                    GL_UNSIGNED_INT, 0, instances.size());
            vao->unbind();
            return;
        }
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(),
                              instances.size());
        vao->unbind();
        return;
    }

    vao->bind();
    shaderProgram.setUniform1i("isInstanced", 0);
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        vao->unbind();
        return;
    }
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    vao->unbind();
}

void CoreObject::setViewMatrix(const glm::mat4 &view) {
    this->view = view;
    for (auto &component : components) {
        component->setViewMatrix(view);
    }
}

void CoreObject::setProjectionMatrix(const glm::mat4 &projection) {
    this->projection = projection;
    for (auto &component : components) {
        component->setProjectionMatrix(projection);
    }
}

CoreObject CoreObject::clone() const {
    CoreObject newObject = *this;

    newObject.vao = nullptr;
    newObject.vbo = nullptr;
    newObject.ebo = nullptr;
    newObject.instanceVBO = nullptr;

    newObject.initialize();

    return newObject;
}

void CoreObject::updateVertices() {
    if (vbo == nullptr || vertices.empty()) {
        throw std::runtime_error("Cannot update vertices: VBO not "
                                 "initialized or empty vertex list");
    }

    vbo->bind();
    vbo->updateData(0, vertices.size() * sizeof(CoreVertex), vertices.data());
    vbo->unbind();
}

void CoreObject::update(Window &window) {
    if (!hasPhysics)
        return;

    this->body->update(window);

    this->position = this->body->position;
    this->rotation = Rotation3d::fromGlmQuat(this->body->orientation);
    updateModelMatrix();
}

void CoreObject::setupPhysics(Body body) {
    this->body = std::make_shared<Body>(body);
    this->body->position = this->position;
    this->body->orientation = this->rotation.toGlmQuat();
    this->hasPhysics = true;
}

void CoreObject::makeEmissive(Scene *scene, Color emissionColor,
                              float intensity) {
    this->initialize();
    if (light != nullptr) {
        throw std::runtime_error("Object is already emissive");
    }
    light = std::make_shared<Light>();
    light->color = emissionColor;
    light->shineColor = emissionColor;
    light->position = this->position;
    light->distance = 10.0f;
    light->doesCastShadows = false;
    this->useDeferredRendering = false;

    for (auto &vertex : vertices) {
        vertex.color = emissionColor * intensity;
    }
    updateVertices();
    useColor = true;
    useTexture = false;

    this->renderOnlyColor();
    this->attachProgram(ShaderProgram::fromDefaultShaders(
        AtlasVertexShader::Color, AtlasFragmentShader::Color));

    scene->addLight(light.get());
}

void CoreObject::updateInstances() {
    if (instances.empty()) {
        return;
    }

    if (this->instanceVBO == nullptr) {
        instanceVBO =
            opal::Buffer::create(opal::BufferUsage::GeneralPurpose,
                                 instances.size() * sizeof(glm::mat4), nullptr);
        auto instanceBindings = makeInstanceAttributeBindings(instanceVBO);
        vao->configureAttributes(instanceBindings);
    }

    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(instances.size());

    for (auto &instance : instances) {
        instance.updateModelMatrix();
        modelMatrices.push_back(instance.getModelMatrix());
    }

    instanceVBO->bind();
    instanceVBO->updateData(0, instances.size() * sizeof(glm::mat4),
                            modelMatrices.data());
    instanceVBO->unbind();
}

void Instance::updateModelMatrix() {
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale.toGlm());

    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.roll)),
                    glm::vec3(0, 0, 1));
    rotation_matrix =
        glm::rotate(rotation_matrix, glm::radians(float(rotation.pitch)),
                    glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(
        rotation_matrix, glm::radians(float(rotation.yaw)), glm::vec3(0, 1, 0));

    glm::mat4 translation_matrix =
        glm::translate(glm::mat4(1.0f), position.toGlm());

    this->model = translation_matrix * rotation_matrix * scale_matrix;
}

void Instance::move(const Position3d &deltaPosition) {
    setPosition(position + deltaPosition);
}

void Instance::setPosition(const Position3d &newPosition) {
    position = newPosition;
    updateModelMatrix();
}

void Instance::setRotation(const Rotation3d &newRotation) {
    rotation = newRotation;
    updateModelMatrix();
}

void Instance::rotate(const Rotation3d &deltaRotation) {
    setRotation(rotation + deltaRotation);
}

void Instance::setScale(const Scale3d &newScale) {
    scale = newScale;
    updateModelMatrix();
}

void Instance::scaleBy(const Scale3d &deltaScale) {
    setScale(Scale3d(scale.x * deltaScale.x, scale.y * deltaScale.y,
                     scale.z * deltaScale.z));
}
