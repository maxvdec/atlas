/*
 shader.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader utilities and functions
 Copyright (c) 2025 maxvdec
*/

#include "atlas/core/shader.h"
#include "atlas/core/default_shaders.h"
#include "atlas/object.h"
#include "atlas/tracer/log.h"
#include "opal/opal.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef VULKAN
namespace {
    bool envFlagEnabled(const char* name) {
        const char* value = std::getenv(name);
        if (!value) {
            return false;
        }
        std::string normalized(value);
        std::transform(normalized.begin(), normalized.end(),
                       normalized.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return normalized == "1" || normalized == "true" ||
               normalized == "yes" || normalized == "on";
    }

    const char* selectVulkanShaderSource(const char* packed,
                                         const char* relativePath) {
        if (envFlagEnabled("ATLAS_VULKAN_FORCE_PACKED_SHADERS")) {
            return packed;
        }

        std::filesystem::path basePath;
        const char* root = std::getenv("ATLAS_SHADER_DIR");
        if (root != nullptr && root[0] != '\0') {
            basePath = root;
        } else {
            basePath = std::filesystem::current_path();
        }

        std::filesystem::path fullPath = basePath / relativePath;
        if (!std::filesystem::exists(fullPath) && (root == nullptr || root[0] == '\0')) {
            std::filesystem::path probe = basePath;
            for (int i = 0; i < 6; ++i) {
                std::filesystem::path candidate = probe / relativePath;
                if (std::filesystem::exists(candidate)) {
                    fullPath = candidate;
                    break;
                }
                if (!probe.has_parent_path()) {
                    break;
                }
                probe = probe.parent_path();
            }
        }

        std::string key = fullPath.lexically_normal().string();

        static std::unordered_map<std::string, std::shared_ptr<std::string>>
            cache;
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second->c_str();
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            return packed;
        }

        auto glsl = std::make_shared<std::string>(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        // Combine GLSL source and packed fallback so Vulkan can try GLSL first
        // and safely fall back to packed SPIR-V if compilation fails.
        auto combined = std::make_shared<std::string>();
        combined->reserve(glsl->size() + (packed ? std::strlen(packed) : 0) + 64);
        combined->append("//__ATLAS_GLSL__\n");
        combined->append(*glsl);
        combined->append("\n//__ATLAS_PACKED__\n");
        if (packed != nullptr) {
            combined->append(packed);
        }

        cache.emplace(key, combined);
        return combined->c_str();
    }
} // namespace

#define ATLAS_VK_SHADER_SOURCE(packed, path) \
    selectVulkanShaderSource((packed), (path))
#endif

#ifndef VULKAN
#define ATLAS_VK_SHADER_SOURCE(packed, path) (packed)
#endif

std::map<std::pair<AtlasVertexShader, AtlasFragmentShader>, ShaderProgram>
ShaderProgram::shaderCache = {};

std::map<AtlasVertexShader, VertexShader> VertexShader::vertexShaderCache = {};

std::map<AtlasFragmentShader, FragmentShader>
FragmentShader::fragmentShaderCache = {};

VertexShader VertexShader::fromDefaultShader(AtlasVertexShader shader) {
    if (VertexShader::vertexShaderCache.find(shader) !=
        VertexShader::vertexShaderCache.end()) {
        return VertexShader::vertexShaderCache[shader];
    }
    VertexShader vertexShader;
    switch (shader) {
    case AtlasVertexShader::Debug:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DEBUG_VERT, "shaders/vulkan/simple/debug.vert"));
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Color:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(COLOR_VERT, "shaders/vulkan/simple/color.vert"));
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Main:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(MAIN_VERT, "shaders/vulkan/main.vert"));
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Lighting, ShaderCapability::Textures,
            ShaderCapability::Shadows, ShaderCapability::EnvironmentMapping,
            ShaderCapability::IBL, ShaderCapability::Material,
            ShaderCapability::Instances, ShaderCapability::Environment
        };
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Texture:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TEXTURE_VERT, "shaders/vulkan/simple/texture.vert"));
        vertexShader.desiredAttributes = {0, 1, 2};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Fullscreen:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(FULLSCREEN_VERT, "shaders/vulkan/fullscreen.vert"));
        vertexShader.desiredAttributes = {0, 1, 2};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Skybox:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(SKYBOX_VERT, "shaders/vulkan/effects/skybox.vert"));
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Depth:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DEPTH_VERT, "shaders/vulkan/shadows/depth.vert"));
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Particle:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(PARTICLE_VERT, "shaders/vulkan/effects/particle.vert"));
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Text:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TEXT_VERT, "shaders/vulkan/ui/text.vert"));
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::PointLightShadow:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(POINT_DEPTH_VERT, "shaders/vulkan/shadows/point_depth.vert"));
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::PointLightShadowNoGeom:
    {
#ifdef VULKAN
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(POINT_DEPTH_NOGEOM_VERT,
                                   "shaders/vulkan/shadows/point_depth_nogeom.vert"));
#else
        vertexShader = VertexShader::fromSource(POINT_DEPTH_VERT);
#endif
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Deferred:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DEFERRED_VERT, "shaders/vulkan/deferred/deferred.vert"));
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Textures, ShaderCapability::Deferred,
            ShaderCapability::Material, ShaderCapability::Instances
        };
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Light:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(LIGHT_VERT, "shaders/vulkan/deferred/light.vert"));
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {
            ShaderCapability::Shadows, ShaderCapability::Lighting,
            ShaderCapability::EnvironmentMapping,
            ShaderCapability::LightDeferred, ShaderCapability::Environment
        };
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Terrain:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TERRAIN_VERT, "shaders/vulkan/terrain/terrain.vert"));
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Volumetric:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(VOLUMETRIC_VERT, "shaders/vulkan/volumetric/volumetric.vert"));
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Fluid:
    {
        vertexShader = VertexShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(FLUID_VERT, "shaders/vulkan/terrain/fluid.vert"));
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Fluid,
            ShaderCapability::Instances
        };
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default vertex shader");
    }

    return vertexShader;
}

VertexShader VertexShader::fromSource(const char* source) {
    VertexShader shader;
    shader.source = source;
    shader.shaderId = 0;
    shader.desiredAttributes = {};
    return shader;
}

void VertexShader::compile() {
    if (shaderId != 0) {
        atlas_warning("Vertex shader already compiled");
        throw std::runtime_error("Vertex shader already compiled");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Vertex);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Vertex shader compilation failed: " +
            std::string(infoLog));
        throw std::runtime_error(
            std::string("Vertex shader compilation failed: ") + infoLog);
    }

    atlas_log("Vertex shader compiled successfully");
    this->shaderId = shader->shaderID;
}

FragmentShader FragmentShader::fromDefaultShader(AtlasFragmentShader shader) {
    if (FragmentShader::fragmentShaderCache.find(shader) !=
        FragmentShader::fragmentShaderCache.end()) {
        return FragmentShader::fragmentShaderCache[shader];
    }
    FragmentShader fragmentShader;
    switch (shader) {
    case AtlasFragmentShader::Debug:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DEBUG_FRAG, "shaders/vulkan/simple/debug.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Color:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(COLOR_FRAG, "shaders/vulkan/simple/color.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Main:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(MAIN_FRAG, "shaders/vulkan/main.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::GaussianBlur:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(GAUSSIAN_FRAG, "shaders/vulkan/effects/gaussian.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Texture:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TEXTURE_FRAG, "shaders/vulkan/simple/texture.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Fullscreen:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(FULLSCREEN_FRAG, "shaders/vulkan/fullscreen.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Skybox:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(SKYBOX_FRAG, "shaders/vulkan/effects/skybox.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Empty:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(EMPTY_FRAG, "shaders/vulkan/shadows/empty.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Particle:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(PARTICLE_FRAG, "shaders/vulkan/effects/particle.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Text:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TEXT_FRAG, "shaders/vulkan/ui/text.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::PointLightShadow:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(POINT_DEPTH_FRAG, "shaders/vulkan/shadows/point_depth.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::PointLightShadowNoGeom:
    {
#ifdef OPENGL
        fragmentShader = FragmentShader::fromSource(EMPTY_FRAG);
#elif defined(VULKAN)
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(POINT_DEPTH_NOGEOM_FRAG,
                                   "shaders/vulkan/shadows/point_depth_nogeom.frag"));
#endif
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Deferred:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DEFERRED_FRAG, "shaders/vulkan/deferred/deferred.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Light:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(LIGHT_FRAG, "shaders/vulkan/deferred/light.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAO:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(SSAO_FRAG, "shaders/vulkan/shadows/ssao.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAOBlur:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(SSAO_BLUR_FRAG, "shaders/vulkan/shadows/ssao_blur.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Terrain:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TERRAIN_FRAG, "shaders/vulkan/terrain/terrain.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Volumetric:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(VOLUMETRIC_FRAG, "shaders/vulkan/volumetric/volumetric.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Downsample:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(DOWNSAMPLE_FRAG, "shaders/vulkan/effects/downsample.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Upsample:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(UPSAMPLE_FRAG, "shaders/vulkan/effects/upsample.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Fluid:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(FLUID_FRAG, "shaders/vulkan/terrain/fluid.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSR:
    {
        fragmentShader = FragmentShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(SSR_FRAG, "shaders/vulkan/effects/ssr.frag"));
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default fragment shader");
    }
    return fragmentShader;
}

FragmentShader FragmentShader::fromSource(const char* source) {
    FragmentShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void FragmentShader::compile() {
    if (shaderId != 0) {
        atlas_warning("Fragment shader already compiled");
        throw std::runtime_error("Fragment shader already compiled");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Fragment);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Fragment shader compilation failed: " +
            std::string(infoLog));
        throw std::runtime_error(
            std::string("Fragment shader compilation failed: ") + infoLog);
    }

    atlas_log("Fragment shader compiled successfully");
    this->shaderId = shader->shaderID;
}

GeometryShader GeometryShader::fromDefaultShader(AtlasGeometryShader shader) {
    switch (shader) {
    case AtlasGeometryShader::PointLightShadow:
        return GeometryShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(POINT_DEPTH_GEOM,
                                   "shaders/vulkan/shadows/point_depth.geom"));
    default:
        throw std::runtime_error("Unknown default geometry shader");
    }
}

GeometryShader GeometryShader::fromSource(const char* source) {
    GeometryShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void GeometryShader::compile() {
    if (shaderId != 0) {
        atlas_warning("Geometry shader already compiled");
        throw std::runtime_error("Geometry shader already compiled");
    }

    shader = opal::Shader::createFromSource(source, opal::ShaderType::Geometry);

    shader->compile();
    bool success = shader->getShaderStatus();

    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        atlas_error("Geometry shader compilation failed: " +
            std::string(infoLog));
        throw std::runtime_error(
            std::string("Geometry shader compilation failed: ") + infoLog);
    }

    atlas_log("Geometry shader compiled successfully");
    this->shaderId = shader->shaderID;
}

TessellationShader
TessellationShader::fromDefaultShader(AtlasTessellationShader shader) {
    switch (shader) {
    case AtlasTessellationShader::TerrainControl:
        return TessellationShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TERRAIN_CONTROL_TESC,
                                   "shaders/vulkan/terrain/terrain_control.tesc"),
            TessellationShaderType::Control);
    case AtlasTessellationShader::TerrainEvaluation:
        return TessellationShader::fromSource(
            ATLAS_VK_SHADER_SOURCE(TERRAIN_EVAL_TESE,
                                   "shaders/vulkan/terrain/terrain_eval.tese"),
            TessellationShaderType::Evaluation);
    default:
        throw std::runtime_error("Unknown default tessellation shader");
    }
}

TessellationShader TessellationShader::fromSource(const char* source,
                                                  TessellationShaderType type) {
    TessellationShader shader;
    shader.source = source;
    shader.type = type;
    shader.shaderId = 0;
    return shader;
}

void TessellationShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Tessellation shader already compiled");
    }

    opal::ShaderType shaderType;
    switch (type) {
    case TessellationShaderType::Control:
        shaderType = opal::ShaderType::TessellationControl;
        break;
    case TessellationShaderType::Evaluation:
        shaderType = opal::ShaderType::TessellationEvaluation;
        break;
    case TessellationShaderType::Primitive:
        throw std::runtime_error("Primitive tessellation shader not supported");
    default:
        throw std::runtime_error("Unknown tessellation shader type");
    }

    shader = opal::Shader::createFromSource(source, shaderType);

    shader->compile();

    bool success = shader->getShaderStatus();
    if (!success) {
        char infoLog[512];
        shader->getShaderLog(infoLog, sizeof(infoLog));
        throw std::runtime_error(
            std::string("Tessellation shader compilation failed: ") + infoLog);
    }

    this->shaderId = shader->shaderID;
}

void ShaderProgram::compile() {
    if (programId != 0) {
        atlas_warning("Shader program already compiled");
        throw std::runtime_error("Shader program already compiled");
    }

    if (vertexShader.shaderId == 0) {
        atlas_error("Vertex shader not compiled");
        throw std::runtime_error("Vertex shader not compiled");
    }

    if (fragmentShader.shaderId == 0) {
        atlas_error("Fragment shader not compiled");
        throw std::runtime_error("Fragment shader not compiled");
    }

    if (fragmentShader.fromDefaultShaderType.has_value() &&
        vertexShader.fromDefaultShaderType.has_value()) {
        auto key = std::make_pair(vertexShader.fromDefaultShaderType.value(),
                                  fragmentShader.fromDefaultShaderType.value());
        if (ShaderProgram::shaderCache.find(key) !=
            ShaderProgram::shaderCache.end()) {
            *this = ShaderProgram::shaderCache[key];
            return;
        }
    }

    atlas_log("Linking shader program");
    desiredAttributes = vertexShader.desiredAttributes;
    capabilities = vertexShader.capabilities;

    this->shader = opal::ShaderProgram::create();

    this->shader->attachShader(vertexShader.shader);
    this->shader->attachShader(fragmentShader.shader);
    if (geometryShader.shaderId != 0) {
        this->shader->attachShader(geometryShader.shader);
    }
    for (const auto& tessShader : tessellationShaders) {
        if (tessShader.shaderId != 0) {
            this->shader->attachShader(tessShader.shader);
        }
    }
    this->shader->link();
    this->programId = this->shader->programID;

    bool success = this->shader->getProgramStatus();
    if (!success) {
        char infoLog[512];
        this->shader->getProgramLog(infoLog, sizeof(infoLog));
        atlas_error("Shader program linking failed: " + std::string(infoLog));
        throw std::runtime_error(
            std::string("Shader program linking failed: ") + infoLog);
    }

    atlas_log("Shader program linked successfully");

    if (fragmentShader.fromDefaultShaderType.has_value() &&
        vertexShader.fromDefaultShaderType.has_value()) {
        auto key = std::make_pair(vertexShader.fromDefaultShaderType.value(),
                                  fragmentShader.fromDefaultShaderType.value());
        ShaderProgram::shaderCache[key] = *this;
    }
}

ShaderProgram ShaderProgram::defaultProgram() {
    static ShaderProgram program;
    static bool initialized = false;

    if (!initialized) {
        program.vertexShader = VertexShader::fromDefaultShader(
            AtlasVertexShader::DEFAULT_VERT_SHADER);
        program.fragmentShader = FragmentShader::fromDefaultShader(
            AtlasFragmentShader::DEFAULT_FRAG_SHADER);
        program.desiredAttributes = program.vertexShader.desiredAttributes;
        program.vertexShader.compile();
        program.fragmentShader.compile();
        program.compile();
        initialized = true;
    }

    return program;
}

void ShaderProgram::setUniform4f(std::string name, float v0, float v1, float v2,
                                 float v3) {
    if (currentPipeline) {
        currentPipeline->setUniform4f(name, v0, v1, v2, v3);
    }
}

void ShaderProgram::setUniform3f(std::string name, float v0, float v1,
                                 float v2) {
    if (currentPipeline) {
        currentPipeline->setUniform3f(name, v0, v1, v2);
    }
}

void ShaderProgram::setUniform2f(std::string name, float v0, float v1) {
    if (currentPipeline) {
        currentPipeline->setUniform2f(name, v0, v1);
    }
}

void ShaderProgram::setUniform1f(std::string name, float v0) {
    if (currentPipeline) {
        currentPipeline->setUniform1f(name, v0);
    }
}

void ShaderProgram::setUniformMat4f(std::string name, const glm::mat4& matrix) {
    if (currentPipeline) {
        currentPipeline->setUniformMat4f(name, matrix);
    }
}

void ShaderProgram::setUniform1i(std::string name, int v0) {
    if (currentPipeline) {
        currentPipeline->setUniform1i(name, v0);
    }
}

void ShaderProgram::setUniformBool(std::string name, bool value) {
    if (currentPipeline) {
        currentPipeline->setUniformBool(name, value);
    }
}

ShaderProgram ShaderProgram::fromDefaultShaders(
    AtlasVertexShader vShader, AtlasFragmentShader fShader,
    GeometryShader gShader, std::vector<TessellationShader> tShaders) {
    ShaderProgram program;
    program.vertexShader = VertexShader::fromDefaultShader(vShader);
    program.fragmentShader = FragmentShader::fromDefaultShader(fShader);
    program.geometryShader = gShader;
    program.tessellationShaders = tShaders;
    program.programId = 0;
    program.desiredAttributes = program.vertexShader.desiredAttributes;

    program.vertexShader.compile();
    program.fragmentShader.compile();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (program.vertexShader.shaderId == 0 ||
        program.fragmentShader.shaderId == 0) {
        throw std::runtime_error("Failed to compile default shaders");
    }

    program.compile();
    return program;
}

std::shared_ptr<opal::Pipeline> ShaderProgram::requestPipeline(
    std::shared_ptr<opal::Pipeline> unbuiltPipeline) {
    unbuiltPipeline->setShaderProgram(this->shader);
    std::vector<LayoutDescriptor> layoutDescriptors =
        CoreVertex::getLayoutDescriptors();

    std::vector<GLuint> activeLocations = this->desiredAttributes;
    if (activeLocations.empty()) {
        for (const auto& attr : layoutDescriptors) {
            activeLocations.push_back(attr.layoutPos);
        }
    }

    std::vector<opal::VertexAttribute> vertexAttributes;
    opal::VertexBinding vertexBinding;

    for (const auto& attr : layoutDescriptors) {
        // Only add attributes that are in the active locations list
        bool isActive = std::find(activeLocations.begin(), activeLocations.end(),
                                  attr.layoutPos) != activeLocations.end();
        if (!isActive) {
            continue;
        }
        vertexAttributes.push_back(opal::VertexAttribute{
            .name = attr.name,
            .type = attr.type,
            .offset = static_cast<uint>(attr.offset),
            .location = static_cast<uint>(attr.layoutPos),
            .normalized = attr.normalized,
            .size = static_cast<uint>(attr.size),
            .stride = static_cast<uint>(attr.stride),
            .inputRate = opal::VertexBindingInputRate::Vertex,
            .divisor = 0
        });
    }

    // Only add instance model attributes if the shader supports instancing
    bool supportsInstancing = std::find(capabilities.begin(), capabilities.end(),
                                        ShaderCapability::Instances) != capabilities.end();
    if (supportsInstancing) {
        std::size_t vec4Size = sizeof(glm::vec4);
        for (unsigned int i = 0; i < 4; ++i) {
            vertexAttributes.push_back(opal::VertexAttribute{
                .name = "instanceModel" + std::to_string(i),
                .type = opal::VertexAttributeType::Float,
                .offset = static_cast<uint>(i * vec4Size),
                .location = static_cast<uint>(6 + i),
                .normalized = false,
                .size = 4,
                .stride = static_cast<uint>(sizeof(glm::mat4)),
                .inputRate = opal::VertexBindingInputRate::Instance,
                .divisor = 1
            });
        }
    }

    vertexBinding = opal::VertexBinding{
        (uint)layoutDescriptors[0].stride,
        opal::VertexBindingInputRate::Vertex
    };

    unbuiltPipeline->setVertexAttributes(vertexAttributes, vertexBinding);

    for (auto& existingPipeline : pipelines) {
        if (*existingPipeline == unbuiltPipeline) {
            currentPipeline = existingPipeline;
            return existingPipeline;
        }
    }

    unbuiltPipeline->build();

    pipelines.push_back(unbuiltPipeline);
    currentPipeline = unbuiltPipeline;

    return unbuiltPipeline;
}
