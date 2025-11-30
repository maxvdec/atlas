/*
 shaders.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader creation functions
 Copyright (c) 2025 maxvdec
*/

#include "opal/opal.h"
#include <cstdint>
#include <glad/glad.h>
#include <memory>
#include <vector>
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

#ifdef OPENGL
uint Shader::getGLShaderType(ShaderType type) {
    switch (type) {
    case ShaderType::Vertex:
        return GL_VERTEX_SHADER;
    case ShaderType::Fragment:
        return GL_FRAGMENT_SHADER;
    case ShaderType::Geometry:
        return GL_GEOMETRY_SHADER;
    case ShaderType::TessellationControl:
        return GL_TESS_CONTROL_SHADER;
    case ShaderType::TessellationEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    default:
        throw std::runtime_error("Unknown shader type");
    }
}
#endif

std::shared_ptr<Shader> Shader::createFromSource(const char *source,
                                                 ShaderType type) {
#ifdef OPENGL
    GLenum shaderType = Shader::getGLShaderType(type);

    uint shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &source, nullptr);

    auto shader = std::make_shared<Shader>();
    shader->shaderID = shaderId;
    shader->type = type;
    shader->source = strdup(source);
    return shader;

#elif defined(VULKAN)
    auto shader = std::make_shared<Shader>();
    shader->type = type;
    shader->source = strdup(source);

    std::vector<uint8_t> bytecode;
    while (*source) {
        uint8_t byte = 0;
        for (int i = 0; i < 2; ++i) {
            char c = *source++;
            byte <<= 4;
            if (c >= '0' && c <= '9') {
                byte |= (c - '0');
            } else if (c >= 'a' && c <= 'f') {
                byte |= (c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                byte |= (c - 'A' + 10);
            } else {
                throw std::runtime_error(
                    "Invalid hex character in shader source");
            }
        }
        bytecode.push_back(byte);
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(bytecode.data());
    if (vkCreateShaderModule(Device::globalDevice, &createInfo, nullptr,
                             &shader->shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shader;
#else
    throw std::runtime_error("Shader creation not implemented for this API");
#endif
}

void Shader::compile() {
#ifdef OPENGL
    glCompileShader(shaderID);
#endif
}

bool Shader::getShaderStatus() const {
#ifdef OPENGL
    GLint success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
#elif defined(VULKAN)
    return true;
#else
    throw std::runtime_error(
        "Shader status retrieval not implemented for this API");
#endif
}

void Shader::getShaderLog(char *logBuffer, size_t bufferSize) const {
#ifdef OPENGL
    glGetShaderInfoLog(shaderID, static_cast<GLsizei>(bufferSize), nullptr,
                       logBuffer);
#elif defined(VULKAN)
    strncpy(logBuffer, "Vulkan shader modules do not have compile logs.",
            bufferSize);
#else
    throw std::runtime_error(
        "Shader log retrieval not implemented for this API");
#endif
}

std::shared_ptr<ShaderProgram> ShaderProgram::create() {
#ifdef OPENGL
    uint programId = glCreateProgram();
    auto program = std::make_shared<ShaderProgram>();
    program->programID = programId;
    program->attachedShaders = {};
    return program;
#elif defined(VULKAN)
    auto program = std::make_shared<ShaderProgram>();
    return program;
#else
    throw std::runtime_error(
        "Shader program creation not implemented for this API");
#endif
}

void ShaderProgram::attachShader(std::shared_ptr<Shader> shader) {
#ifdef OPENGL
    glAttachShader(programID, shader->shaderID);
    attachedShaders.push_back(shader);
#elif defined(VULKAN)
    attachedShaders.push_back(shader);
#else
    throw std::runtime_error("Shader attachment not implemented for this API");
#endif
}

void ShaderProgram::link() {
#ifdef OPENGL
    glLinkProgram(programID);
#elif defined(VULKAN)

#else
    throw std::runtime_error(
        "Shader program linking not implemented for this API");
#endif
}

bool ShaderProgram::getProgramStatus() const {
#ifdef OPENGL
    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    return success == GL_TRUE;
#elif defined(VULKAN)
    return true;
#else
    throw std::runtime_error(
        "Shader program status retrieval not implemented for this API");
#endif
}

void ShaderProgram::getProgramLog(char *logBuffer, size_t bufferSize) const {
#ifdef OPENGL
    glGetProgramInfoLog(programID, static_cast<GLsizei>(bufferSize), nullptr,
                        logBuffer);
#elif defined(VULKAN)
    strncpy(logBuffer, "Vulkan shader programs do not have link logs.",
            bufferSize);
#else
    throw std::runtime_error(
        "Shader program log retrieval not implemented for this API");
#endif
}

} // namespace opal
