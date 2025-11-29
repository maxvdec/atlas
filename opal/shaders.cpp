/*
 shaders.cpp
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader creation functions
 Copyright (c) 2025 maxvdec
*/

#include "opal/opal.h"
#include <glad/glad.h>
#include <memory>

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
#else
    throw std::runtime_error(
        "Shader status retrieval not implemented for this API");
#endif
}

void Shader::getShaderLog(char *logBuffer, size_t bufferSize) const {
#ifdef OPENGL
    glGetShaderInfoLog(shaderID, static_cast<GLsizei>(bufferSize), nullptr,
                       logBuffer);
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
#else
    throw std::runtime_error(
        "Shader program creation not implemented for this API");
#endif
}

void ShaderProgram::attachShader(std::shared_ptr<Shader> shader) {
#ifdef OPENGL
    glAttachShader(programID, shader->shaderID);
    attachedShaders.push_back(shader);
#else
    throw std::runtime_error("Shader attachment not implemented for this API");
#endif
}

void ShaderProgram::link() {
#ifdef OPENGL
    glLinkProgram(programID);
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
#else
    throw std::runtime_error(
        "Shader program status retrieval not implemented for this API");
#endif
}

void ShaderProgram::getProgramLog(char *logBuffer, size_t bufferSize) const {
#ifdef OPENGL
    glGetProgramInfoLog(programID, static_cast<GLsizei>(bufferSize), nullptr,
                        logBuffer);
#else
    throw std::runtime_error(
        "Shader program log retrieval not implemented for this API");
#endif
}

} // namespace opal
