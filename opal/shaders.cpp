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
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
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

int Shader::currentId = 1;
int ShaderProgram::currentId = 1;

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
    if (type == ShaderType::Geometry) {
        throw std::runtime_error(
            "Geometry shaders are not supported in Vulkan");
    }

    shader->spirvBytecode.resize(bytecode.size() / 4);
    memcpy(shader->spirvBytecode.data(), bytecode.data(), bytecode.size());

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(bytecode.data());
    if (vkCreateShaderModule(Device::globalDevice, &createInfo, nullptr,
                             &shader->shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    shader->performReflection();

    return shader;
#else
    throw std::runtime_error("Shader creation not implemented for this API");
#endif
}

void Shader::compile() {
#ifdef OPENGL
    glCompileShader(shaderID);
#elif defined(VULKAN)
    this->shaderID = Shader::currentId++;
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
    for (const auto &pair : shader->uniformBindings) {
        uniformBindings[pair.first] = pair.second;
    }
#else
    throw std::runtime_error("Shader attachment not implemented for this API");
#endif
}

void ShaderProgram::link() {
#ifdef OPENGL
    glLinkProgram(programID);
#elif defined(VULKAN)
    this->programID = ShaderProgram::currentId++;
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

#ifdef VULKAN
void Shader::performReflection() {
    if (spirvBytecode.empty()) {
        return;
    }

    spirv_cross::Compiler compiler(spirvBytecode);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for (const auto &ubo : resources.uniform_buffers) {
        uint32_t set =
            compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(ubo.id, spv::DecorationBinding);

        const spirv_cross::SPIRType &type = compiler.get_type(ubo.base_type_id);
        size_t blockSize = compiler.get_declared_struct_size(type);

        UniformBindingInfo blockInfo;
        blockInfo.set = set;
        blockInfo.binding = binding;
        blockInfo.size = static_cast<uint32_t>(blockSize);
        blockInfo.offset = 0;
        blockInfo.isSampler = false;
        blockInfo.isBuffer = true;
        blockInfo.isStorageBuffer = false;
        uniformBindings[ubo.name] = blockInfo;

        for (uint32_t i = 0; i < type.member_types.size(); ++i) {
            std::string memberName =
                compiler.get_member_name(ubo.base_type_id, i);
            uint32_t memberOffset = compiler.type_struct_member_offset(type, i);
            const spirv_cross::SPIRType &memberType =
                compiler.get_type(type.member_types[i]);
            size_t memberSize =
                compiler.get_declared_struct_member_size(type, i);

            UniformBindingInfo memberInfo;
            memberInfo.set = set;
            memberInfo.binding = binding;
            memberInfo.size = static_cast<uint32_t>(memberSize);
            memberInfo.offset = memberOffset;
            memberInfo.isSampler = false;
            memberInfo.isBuffer = true;
            memberInfo.isStorageBuffer = false;

            uniformBindings[ubo.name + "." + memberName] = memberInfo;
            if (uniformBindings.find(memberName) == uniformBindings.end()) {
                uniformBindings[memberName] = memberInfo;
            }
        }
    }

    for (const auto &pc : resources.push_constant_buffers) {
        const spirv_cross::SPIRType &type = compiler.get_type(pc.base_type_id);

        for (uint32_t i = 0; i < type.member_types.size(); ++i) {
            std::string memberName =
                compiler.get_member_name(pc.base_type_id, i);
            uint32_t memberOffset = compiler.type_struct_member_offset(type, i);
            size_t memberSize =
                compiler.get_declared_struct_member_size(type, i);

            UniformBindingInfo memberInfo;
            memberInfo.set = 0;
            memberInfo.binding = 0;
            memberInfo.size = static_cast<uint32_t>(memberSize);
            memberInfo.offset = memberOffset;
            memberInfo.isSampler = false;
            memberInfo.isBuffer = false;
            memberInfo.isStorageBuffer = false;

            uniformBindings[memberName] = memberInfo;
            if (!pc.name.empty()) {
                uniformBindings[pc.name + "." + memberName] = memberInfo;
            }
        }
    }

    for (const auto &sampler : resources.sampled_images) {
        uint32_t set =
            compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(sampler.id, spv::DecorationBinding);

        UniformBindingInfo samplerInfo;
        samplerInfo.set = set;
        samplerInfo.binding = binding;
        samplerInfo.size = 0;
        samplerInfo.offset = 0;
        samplerInfo.isSampler = true;
        samplerInfo.isBuffer = false;
        samplerInfo.isStorageBuffer = false;
        uniformBindings[sampler.name] = samplerInfo;
    }

    for (const auto &sampler : resources.separate_samplers) {
        uint32_t set =
            compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(sampler.id, spv::DecorationBinding);

        UniformBindingInfo samplerInfo;
        samplerInfo.set = set;
        samplerInfo.binding = binding;
        samplerInfo.size = 0;
        samplerInfo.offset = 0;
        samplerInfo.isSampler = true;
        samplerInfo.isBuffer = false;
        samplerInfo.isStorageBuffer = false;
        uniformBindings[sampler.name] = samplerInfo;
    }

    for (const auto &image : resources.separate_images) {
        uint32_t set =
            compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(image.id, spv::DecorationBinding);

        UniformBindingInfo imageInfo;
        imageInfo.set = set;
        imageInfo.binding = binding;
        imageInfo.size = 0;
        imageInfo.offset = 0;
        imageInfo.isSampler = true;
        imageInfo.isBuffer = false;
        imageInfo.isStorageBuffer = false;
        uniformBindings[image.name] = imageInfo;
    }

    for (const auto &ssbo : resources.storage_buffers) {
        uint32_t set =
            compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
        uint32_t binding =
            compiler.get_decoration(ssbo.id, spv::DecorationBinding);

        UniformBindingInfo ssboInfo;
        ssboInfo.set = set;
        ssboInfo.binding = binding;
        ssboInfo.size = 0;
        ssboInfo.offset = 0;
        ssboInfo.isSampler = false;
        ssboInfo.isBuffer = true;
        ssboInfo.isStorageBuffer = true;
        uniformBindings[ssbo.name] = ssboInfo;
    }
}

const UniformBindingInfo *
ShaderProgram::findUniform(const std::string &name) const {
    auto it = uniformBindings.find(name);
    if (it != uniformBindings.end()) {
        return &it->second;
    }
    return nullptr;
}
#endif

} // namespace opal
