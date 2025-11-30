//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for Vulkan
// Copyright (c) 2025 Max Van den Eynde
//

#ifdef VULKAN
#include <opal/opal.h>
#include <vulkan/vulkan.hpp>

namespace opal {

VkPipelineShaderStageCreateInfo Shader::makeShaderStageInfo() const {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    switch (this->type) {
    case ShaderType::Vertex:
        shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderType::Fragment:
        shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    case ShaderType::Geometry:
        shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        break;
    case ShaderType::TessellationControl:
        shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        break;
    case ShaderType::TessellationEvaluation:
        shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        break;
    default:
        throw std::runtime_error("Unknown shader type");
    }

    shaderStageInfo.module = this->shaderModule;
    shaderStageInfo.pName = "main";

    return shaderStageInfo;
}

} // namespace opal
#endif