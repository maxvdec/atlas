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
#include "atlas/units.h"
#include <glad/glad.h>
#include <map>
#include <string>
#include <thread>
#include <utility>

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
    case AtlasVertexShader::Debug: {
        vertexShader = VertexShader::fromSource(DEBUG_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Color: {
        vertexShader = VertexShader::fromSource(COLOR_VERT);
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Main: {
        vertexShader = VertexShader::fromSource(MAIN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3};
        vertexShader.capabilities = {
            ShaderCapability::Lighting, ShaderCapability::Textures,
            ShaderCapability::Shadows,  ShaderCapability::EnvironmentMapping,
            ShaderCapability::IBL,      ShaderCapability::Material,
            ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Texture: {
        vertexShader = VertexShader::fromSource(TEXTURE_VERT);
        vertexShader.desiredAttributes = {0, 1, 2};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Fullscreen: {
        vertexShader = VertexShader::fromSource(FULLSCREEN_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Skybox: {
        vertexShader = VertexShader::fromSource(SKYBOX_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Depth: {
        vertexShader = VertexShader::fromSource(DEPTH_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Particle: {
        vertexShader = VertexShader::fromSource(PARTICLE_VERT);
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Text: {
        vertexShader = VertexShader::fromSource(TEXT_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {ShaderCapability::Textures};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::PointLightShadow: {
        vertexShader = VertexShader::fromSource(POINT_DEPTH_VERT);
        vertexShader.desiredAttributes = {0};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Deferred: {
        vertexShader = VertexShader::fromSource(DEFERRED_VERT);
        vertexShader.desiredAttributes = {0, 1, 2, 3, 4, 5};
        vertexShader.capabilities = {
            ShaderCapability::Textures, ShaderCapability::Deferred,
            ShaderCapability::Material, ShaderCapability::Instances};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Light: {
        vertexShader = VertexShader::fromSource(LIGHT_VERT);
        vertexShader.desiredAttributes = {0, 1};
        vertexShader.capabilities = {ShaderCapability::Shadows,
                                     ShaderCapability::Lighting,
                                     ShaderCapability::EnvironmentMapping,
                                     ShaderCapability::LightDeferred};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Terrain: {
        vertexShader = VertexShader::fromSource(TERRAIN_VERT);
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    case AtlasVertexShader::Volumetric: {
        vertexShader = VertexShader::fromSource(VOLUMETRIC_VERT);
        vertexShader.desiredAttributes = {};
        vertexShader.capabilities = {};
        vertexShader.fromDefaultShaderType = shader;
        VertexShader::vertexShaderCache[shader] = vertexShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default vertex shader");
    }

    return vertexShader;
}

VertexShader VertexShader::fromSource(const char *source) {
    VertexShader shader;
    shader.source = source;
    shader.shaderId = 0;
    shader.desiredAttributes = {};
    return shader;
}

void VertexShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Vertex shader already compiled");
    }

    shaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Vertex shader compilation failed: ") + infoLog);
    }
}

FragmentShader FragmentShader::fromDefaultShader(AtlasFragmentShader shader) {
    if (FragmentShader::fragmentShaderCache.find(shader) !=
        FragmentShader::fragmentShaderCache.end()) {
        return FragmentShader::fragmentShaderCache[shader];
    }
    FragmentShader fragmentShader;
    switch (shader) {
    case AtlasFragmentShader::Debug: {
        fragmentShader = FragmentShader::fromSource(DEBUG_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Color: {
        fragmentShader = FragmentShader::fromSource(COLOR_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Main: {
        fragmentShader = FragmentShader::fromSource(MAIN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::GaussianBlur: {
        fragmentShader = FragmentShader::fromSource(GAUSSIAN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Texture: {
        fragmentShader = FragmentShader::fromSource(TEXTURE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Fullscreen: {
        fragmentShader = FragmentShader::fromSource(FULLSCREEN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Skybox: {
        fragmentShader = FragmentShader::fromSource(SKYBOX_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Empty: {
        fragmentShader = FragmentShader::fromSource(EMPTY_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Particle: {
        fragmentShader = FragmentShader::fromSource(PARTICLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Text: {
        fragmentShader = FragmentShader::fromSource(TEXT_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::PointLightShadow: {
        fragmentShader = FragmentShader::fromSource(POINT_DEPTH_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Deferred: {
        fragmentShader = FragmentShader::fromSource(DEFERRED_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Light: {
        fragmentShader = FragmentShader::fromSource(LIGHT_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAO: {
        fragmentShader = FragmentShader::fromSource(SSAO_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::SSAOBlur: {
        fragmentShader = FragmentShader::fromSource(SSAO_BLUR_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Terrain: {
        fragmentShader = FragmentShader::fromSource(TERRAIN_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Volumetric: {
        fragmentShader = FragmentShader::fromSource(VOLUMETRIC_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Downsample: {
        fragmentShader = FragmentShader::fromSource(DOWNSAMPLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    case AtlasFragmentShader::Upsample: {
        fragmentShader = FragmentShader::fromSource(UPSAMPLE_FRAG);
        fragmentShader.fromDefaultShaderType = shader;
        fragmentShaderCache[shader] = fragmentShader;
        break;
    }
    default:
        throw std::runtime_error("Unknown default fragment shader");
    }
    return fragmentShader;
}

FragmentShader FragmentShader::fromSource(const char *source) {
    FragmentShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void FragmentShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Fragment shader already compiled");
    }

    shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Fragment shader compilation failed: ") + infoLog);
    }
}

GeometryShader GeometryShader::fromDefaultShader(AtlasGeometryShader shader) {
    switch (shader) {
    case AtlasGeometryShader::PointLightShadow:
        return GeometryShader::fromSource(POINT_DEPTH_GEOM);
    default:
        throw std::runtime_error("Unknown default geometry shader");
    }
}

GeometryShader GeometryShader::fromSource(const char *source) {
    GeometryShader shader;
    shader.source = source;
    shader.shaderId = 0;
    return shader;
}

void GeometryShader::compile() {
    if (shaderId != 0) {
        throw std::runtime_error("Geometry shader already compiled");
    }

    shaderId = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Geometry shader compilation failed: ") + infoLog);
    }
}

TessellationShader
TessellationShader::fromDefaultShader(AtlasTessellationShader shader) {
    switch (shader) {
    case AtlasTessellationShader::TerrainControl:
        return TessellationShader::fromSource(TERRAIN_CONTROL_TESC,
                                              TessellationShaderType::Control);
    case AtlasTessellationShader::TerrainEvaluation:
        return TessellationShader::fromSource(
            TERRAIN_EVAL_TESE, TessellationShaderType::Evaluation);
    default:
        throw std::runtime_error("Unknown default tessellation shader");
    }
}

TessellationShader TessellationShader::fromSource(const char *source,
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

    GLenum shaderType;
    switch (type) {
    case TessellationShaderType::Control:
        shaderType = GL_TESS_CONTROL_SHADER;
        break;
    case TessellationShaderType::Evaluation:
        shaderType = GL_TESS_EVALUATION_SHADER;
        break;
    case TessellationShaderType::Primitive:
        throw std::runtime_error("Primitive tessellation shader not supported");
    default:
        throw std::runtime_error("Unknown tessellation shader type");
    }

    shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Tessellation shader compilation failed: ") + infoLog);
    }
}

void ShaderProgram::compile() {
    if (programId != 0) {
        throw std::runtime_error("Shader program already compiled");
    }

    if (vertexShader.shaderId == 0) {
        throw std::runtime_error("Vertex shader not compiled");
    }

    if (fragmentShader.shaderId == 0) {
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

    desiredAttributes = vertexShader.desiredAttributes;
    capabilities = vertexShader.capabilities;

    programId = glCreateProgram();
    glAttachShader(programId, vertexShader.shaderId);
    glAttachShader(programId, fragmentShader.shaderId);
    if (geometryShader.shaderId != 0) {
        glAttachShader(programId, geometryShader.shaderId);
    }
    for (const auto &tessShader : tessellationShaders) {
        if (tessShader.shaderId != 0) {
            glAttachShader(programId, tessShader.shaderId);
        }
    }
    glLinkProgram(programId);

    GLint success;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, nullptr, infoLog);
        throw std::runtime_error(
            std::string("Shader program linking failed: ") + infoLog);
    }

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
    glUniform4f(glGetUniformLocation(programId, name.c_str()), v0, v1, v2, v3);
}

void ShaderProgram::setUniform3f(std::string name, float v0, float v1,
                                 float v2) {
    glUniform3f(glGetUniformLocation(programId, name.c_str()), v0, v1, v2);
}

void ShaderProgram::setUniform2f(std::string name, float v0, float v1) {
    glUniform2f(glGetUniformLocation(programId, name.c_str()), v0, v1);
}

void ShaderProgram::setUniform1f(std::string name, float v0) {
    glUniform1f(glGetUniformLocation(programId, name.c_str()), v0);
}

void ShaderProgram::setUniformMat4f(std::string name, const glm::mat4 &matrix) {
    glUniformMatrix4fv(glGetUniformLocation(programId, name.c_str()), 1,
                       GL_FALSE, &matrix[0][0]);
}

void ShaderProgram::setUniform1i(std::string name, int v0) {
    glUniform1i(glGetUniformLocation(programId, name.c_str()), v0);
}

void ShaderProgram::setUniformBool(std::string name, bool value) {
    glUniform1i(glGetUniformLocation(programId, name.c_str()), (int)value);
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

    // We need to wait a bit to ensure the GPU is ready for the next compilation
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (program.vertexShader.shaderId == 0 ||
        program.fragmentShader.shaderId == 0) {
        throw std::runtime_error("Failed to compile default shaders");
    }

    program.compile();
    return program;
}
