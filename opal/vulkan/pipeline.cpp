//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for Vulkan
// Copyright (c) 2025 Max Van den Eynde
//

#include <map>
#include <memory>
#include <vector>
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

VkFormat Pipeline::getFormat(VertexAttributeType type, uint size,
                             bool normalized) const {
    switch (type) {
    case VertexAttributeType::Float:
        switch (size) {
        case 1:
            return VK_FORMAT_R32_SFLOAT;
        case 2:
            return VK_FORMAT_R32G32_SFLOAT;
        case 3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            break;
        }
        break;
    case VertexAttributeType::Int:
        switch (size) {
        case 1:
            return VK_FORMAT_R32_SINT;
        case 2:
            return VK_FORMAT_R32G32_SINT;
        case 3:
            return VK_FORMAT_R32G32B32_SINT;
        case 4:
            return VK_FORMAT_R32G32B32A32_SINT;
        default:
            break;
        }
        break;
    case VertexAttributeType::UnsignedInt:
        switch (size) {
        case 1:
            return VK_FORMAT_R32_UINT;
        case 2:
            return VK_FORMAT_R32G32_UINT;
        case 3:
            return VK_FORMAT_R32G32B32_UINT;
        case 4:
            return VK_FORMAT_R32G32B32A32_UINT;
        default:
            break;
        }
        break;
    case VertexAttributeType::Short:
        if (normalized)
            return (size == 2) ? VK_FORMAT_R16G16_SNORM
                               : VK_FORMAT_R16G16B16A16_SNORM;
        else
            return (size == 2) ? VK_FORMAT_R16G16_SINT
                               : VK_FORMAT_R16G16B16A16_SINT;
    case VertexAttributeType::UnsignedShort:
        if (normalized)
            return (size == 2) ? VK_FORMAT_R16G16_UNORM
                               : VK_FORMAT_R16G16B16A16_UNORM;
        else
            return (size == 2) ? VK_FORMAT_R16G16_UINT
                               : VK_FORMAT_R16G16B16A16_UINT;
    case VertexAttributeType::Byte:
        if (normalized)
            return VK_FORMAT_R8G8B8A8_SNORM;
        else
            return VK_FORMAT_R8G8B8A8_SINT;
    case VertexAttributeType::UnsignedByte:
        if (normalized)
            return VK_FORMAT_R8G8B8A8_UNORM;
        else
            return VK_FORMAT_R8G8B8A8_UINT;
    case VertexAttributeType::Double:
        switch (size) {
        case 1:
            return VK_FORMAT_R64_SFLOAT;
        case 2:
            return VK_FORMAT_R64G64_SFLOAT;
        case 3:
            return VK_FORMAT_R64G64B64_SFLOAT;
        case 4:
            return VK_FORMAT_R64G64B64A64_SFLOAT;
        default:
            break;
        }
        break;
    }
    return VK_FORMAT_UNDEFINED;
}

void Pipeline::buildPipelineLayout() {
    vkDynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(vkDynamicStates.size());
    dynamicState.pDynamicStates = vkDynamicStates.data();

    vertexInputInfo = {};
    vkBindingDescriptions.clear();
    vkAttributeDescriptions.clear();

    VkVertexInputBindingDescription vertexBindingDesc{};
    vertexBindingDesc.binding = 0;
    vertexBindingDesc.stride = vertexBinding.stride;
    vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vkBindingDescriptions.push_back(vertexBindingDesc);

    bool foundInstanceAttributes = false;
    uint32_t instanceStride = 0;
    for (const auto &attr : this->vertexAttributes) {
        if (attr.inputRate == VertexBindingInputRate::Instance) {
            foundInstanceAttributes = true;
            instanceStride = attr.stride;
            break;
        }
    }

    if (foundInstanceAttributes) {
        VkVertexInputBindingDescription instanceBindingDesc{};
        instanceBindingDesc.binding = 1;
        instanceBindingDesc.stride = instanceStride;
        instanceBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        vkBindingDescriptions.push_back(instanceBindingDesc);
    }

    hasInstanceAttributes = foundInstanceAttributes;

    for (const auto &attr : this->vertexAttributes) {
        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.location = attr.location;
        attrDesc.binding =
            (attr.inputRate == VertexBindingInputRate::Instance) ? 1 : 0;
        attrDesc.format =
            this->getFormat(attr.type, attr.size, attr.normalized);
        attrDesc.offset = attr.offset;

        vkAttributeDescriptions.push_back(attrDesc);
    }

    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount =
        static_cast<uint32_t>(vkBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = vkBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vkAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions =
        vkAttributeDescriptions.data();

    inputAssembly = {};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (this->primitiveStyle) {
    case PrimitiveStyle::Points:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case PrimitiveStyle::Lines:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case PrimitiveStyle::LineStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        break;
    case PrimitiveStyle::Triangles:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case PrimitiveStyle::TriangleStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;
    case PrimitiveStyle::TriangleFan:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        break;
    case PrimitiveStyle::Patches:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        break;
    default:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    }

    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vkViewport = {};
    vkViewport.x = static_cast<float>(viewportX);
    vkViewport.y = static_cast<float>(viewportY);
    vkViewport.width = static_cast<float>(viewportWidth);
    vkViewport.height = static_cast<float>(viewportHeight);
    vkViewport.minDepth = 0.0f;
    vkViewport.maxDepth = 1.0f;

    vkScissor = {};
    vkScissor.offset = {.x = 0, .y = 0};
    vkScissor.extent = {.width = static_cast<uint32_t>(viewportWidth),
                        .height = static_cast<uint32_t>(viewportHeight)};

    viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &vkViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &vkScissor;

    rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    switch (this->rasterizerMode) {
    case RasterizerMode::Fill:
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        break;
    case RasterizerMode::Line:
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        break;
    case RasterizerMode::Point:
        rasterizer.polygonMode = VK_POLYGON_MODE_POINT;
        break;
    default:
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        break;
    }

    rasterizer.lineWidth = this->lineWidth;

    switch (this->cullMode) {
    case CullMode::None:
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        break;
    case CullMode::Front:
        rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        break;
    case CullMode::Back:
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    case CullMode::FrontAndBack:
        rasterizer.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
        break;
    default:
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    }

    switch (this->frontFace) {
    case FrontFace::Clockwise:
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        break;
    case FrontFace::CounterClockwise:
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        break;
    }

    rasterizer.depthBiasEnable =
        this->polygonOffsetEnabled ? VK_TRUE : VK_FALSE;
    rasterizer.depthBiasConstantFactor = this->polygonOffsetFactor;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = this->polygonOffsetUnits;

    // Multisampling
    if (this->multisamplingEnabled) {
        multisampling = {};
        multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
    }

    depthStencil = {};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = this->depthTestEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable =
        this->depthWriteEnabled ? VK_TRUE : VK_FALSE;
    switch (this->depthCompareOp) {
    case CompareOp::Never:
        depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        break;
    case CompareOp::Less:
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        break;
    case CompareOp::Equal:
        depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
        break;
    case CompareOp::LessEqual:
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        break;
    case CompareOp::Greater:
        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
        break;
    case CompareOp::NotEqual:
        depthStencil.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
        break;
    case CompareOp::GreaterEqual:
        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
        break;
    case CompareOp::Always:
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        break;
    }

    vkColorBlendAttachments.clear();
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable =
        this->blendingEnabled ? VK_TRUE : VK_FALSE;
    switch (this->blendSrcFactor) {
    case BlendFunc::Zero:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
    case BlendFunc::One:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case BlendFunc::SrcColor:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        break;
    case BlendFunc::OneMinusSrcColor:
        colorBlendAttachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        break;
    case BlendFunc::DstColor:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        break;
    case BlendFunc::OneMinusDstColor:
        colorBlendAttachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        break;
    case BlendFunc::SrcAlpha:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        break;
    case BlendFunc::OneMinusSrcAlpha:
        colorBlendAttachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BlendFunc::DstAlpha:
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        break;
    case BlendFunc::OneMinusDstAlpha:
        colorBlendAttachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        break;
    }

    switch (this->blendDstFactor) {
    case BlendFunc::Zero:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
    case BlendFunc::One:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case BlendFunc::SrcColor:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        break;
    case BlendFunc::OneMinusSrcColor:
        colorBlendAttachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        break;
    case BlendFunc::DstColor:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        break;
    case BlendFunc::OneMinusDstColor:
        colorBlendAttachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        break;
    case BlendFunc::SrcAlpha:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        break;
    case BlendFunc::OneMinusSrcAlpha:
        colorBlendAttachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BlendFunc::DstAlpha:
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        break;
    case BlendFunc::OneMinusDstAlpha:
        colorBlendAttachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        break;
    }

    switch (this->blendEquation) {
    case BlendEquation::Add:
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case BlendEquation::Subtract:
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_SUBTRACT;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
        break;
    case BlendEquation::ReverseSubtract:
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        break;
    case BlendEquation::Min:
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_MIN;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MIN;
        break;
    case BlendEquation::Max:
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_MAX;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;
        break;
    };

    vkColorBlendAttachments.push_back(colorBlendAttachment);

    colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount =
        static_cast<uint32_t>(vkColorBlendAttachments.size());
    colorBlending.pAttachments = vkColorBlendAttachments.data();

    uint32_t maxPushConstantSize = 0;
    VkShaderStageFlags pushConstantStageFlags = 0;

    descriptorBindingInfo.clear();

    if (this->shaderProgram) {
        for (const auto &shader : this->shaderProgram->attachedShaders) {
            VkShaderStageFlags stageFlag = 0;
            switch (shader->type) {
            case ShaderType::Vertex:
                stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderType::Fragment:
                stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderType::Geometry:
                stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ShaderType::TessellationControl:
                stageFlag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case ShaderType::TessellationEvaluation:
                stageFlag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            default:
                break;
            }

            for (const auto &pair : shader->uniformBindings) {
                const UniformBindingInfo &info = pair.second;
                if (info.isBuffer) {
                    // This is a buffer - add to descriptor set layout
                    auto &binding =
                        descriptorBindingInfo[info.set][info.binding];
                    if (info.isStorageBuffer) {
                        binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    } else {
                        binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    }
                    binding.stageFlags |= stageFlag;
                    binding.count = 1;
                    binding.minBufferSize =
                        std::max(binding.minBufferSize, info.size);
                    binding.isBuffer = true;
                    binding.isSampler = false;
                } else if (info.isSampler) {
                    auto &binding =
                        descriptorBindingInfo[info.set][info.binding];
                    binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    binding.stageFlags |= stageFlag;
                    binding.count = 1;
                    binding.isBuffer = false;
                    binding.isSampler = true;
                    binding.isCubemap = info.isCubemap;
                } else {
                    uint32_t endOffset = info.offset + info.size;
                    if (endOffset > maxPushConstantSize) {
                        maxPushConstantSize = endOffset;
                    }
                    pushConstantStageFlags |= stageFlag;
                }
            }
        }
    }

    resetDescriptorSets();

    for (auto layout : descriptorSetLayouts) {
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(Device::globalDevice, layout, nullptr);
        }
    }
    descriptorSetLayouts.clear();

    if (!descriptorBindingInfo.empty()) {
        uint32_t maxSet = descriptorBindingInfo.rbegin()->first;
        descriptorSetLayouts.resize(maxSet + 1, VK_NULL_HANDLE);

        for (const auto &setPair : descriptorBindingInfo) {
            uint32_t setIndex = setPair.first;
            const auto &bindings = setPair.second;

            std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
            std::vector<VkDescriptorBindingFlags> bindingFlags;
            for (const auto &bindingPair : bindings) {
                VkDescriptorSetLayoutBinding layoutBinding{};
                layoutBinding.binding = bindingPair.first;
                layoutBinding.descriptorType = bindingPair.second.type;
                layoutBinding.descriptorCount = bindingPair.second.count;
                layoutBinding.stageFlags = bindingPair.second.stageFlags;
                layoutBinding.pImmutableSamplers = nullptr;
                layoutBindings.push_back(layoutBinding);
                bindingFlags.push_back(
                    VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
            }

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
            bindingFlagsInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingFlagsInfo.bindingCount =
                static_cast<uint32_t>(bindingFlags.size());
            bindingFlagsInfo.pBindingFlags = bindingFlags.data();

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount =
                static_cast<uint32_t>(layoutBindings.size());
            layoutInfo.pBindings = layoutBindings.data();
            layoutInfo.flags =
                VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            layoutInfo.pNext = &bindingFlagsInfo;

            if (vkCreateDescriptorSetLayout(
                    Device::globalDevice, &layoutInfo, nullptr,
                    &descriptorSetLayouts[setIndex]) != VK_SUCCESS) {
                throw std::runtime_error(
                    "Failed to create descriptor set layout!");
            }
        }
    }

    this->pushConstantSize = maxPushConstantSize;
    this->pushConstantStages = pushConstantStageFlags;
    if (maxPushConstantSize > 0) {
        this->pushConstantData.resize(maxPushConstantSize, 0);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (!descriptorSetLayouts.empty()) {
        pipelineLayoutInfo.setLayoutCount =
            static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    }

    VkPushConstantRange pushConstantRange{};
    if (maxPushConstantSize > 0) {
        pushConstantRange.stageFlags = pushConstantStageFlags;
        pushConstantRange.offset = 0;
        pushConstantRange.size = maxPushConstantSize;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    if (vkCreatePipelineLayout(Device::globalDevice, &pipelineLayoutInfo,
                               nullptr, &this->pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    ensureDescriptorResources();
}

VkFormat opalFormatToVkFormat(opal::TextureFormat format) {
    switch (format) {
    case opal::TextureFormat::Rgba8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case opal::TextureFormat::sRgba8:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case opal::TextureFormat::Rgb8:
        return VK_FORMAT_R8G8B8_UNORM;
    case opal::TextureFormat::sRgb8:
        return VK_FORMAT_R8G8B8_SRGB;
    case opal::TextureFormat::Rgba16F:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case opal::TextureFormat::Rgb16F:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case opal::TextureFormat::Depth24Stencil8:
    case opal::TextureFormat::DepthComponent24:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case opal::TextureFormat::Depth32F:
        return VK_FORMAT_D32_SFLOAT;
    case opal::TextureFormat::Red8:
        return VK_FORMAT_R8_UNORM;
    case opal::TextureFormat::Red16F:
        return VK_FORMAT_R16_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

std::shared_ptr<CoreRenderPass>
CoreRenderPass::create(std::shared_ptr<Pipeline> pipeline,
                       std::shared_ptr<Framebuffer> framebuffer) {
    auto renderPass = std::make_shared<CoreRenderPass>();
    renderPass->opalPipeline = pipeline;
    renderPass->opalFramebuffer = framebuffer;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentReference depthAttachmentRef{};
    bool hasDepthAttachment = false;

    if (framebuffer->isDefaultFramebuffer) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = Device::globalInstance->swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(colorRef);

        VkAttachmentDescription brightAttachment{};
        brightAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        brightAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        brightAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        brightAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        brightAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        brightAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        brightAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        brightAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(brightAttachment);

        VkAttachmentReference brightRef{};
        brightRef.attachment = 1;
        brightRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(brightRef);

        if (Device::globalInstance->swapChainDepthTexture != nullptr) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = VK_FORMAT_D32_SFLOAT;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);

            depthAttachmentRef.attachment = 2; // After color and bright
            depthAttachmentRef.layout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepthAttachment = true;
        }
    } else {
        for (const auto &attachment : framebuffer->attachments) {
            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format =
                opalFormatToVkFormat(attachment.texture->format);
            attachmentDesc.samples = attachment.texture->samples > 1
                                         ? VK_SAMPLE_COUNT_4_BIT
                                         : VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (attachment.type == opal::Attachment::Type::Color) {
                attachmentDesc.finalLayout =
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            } else if (attachment.type == opal::Attachment::Type::Depth ||
                       attachment.type ==
                           opal::Attachment::Type::DepthStencil) {
                attachmentDesc.finalLayout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            }

            attachments.push_back(attachmentDesc);
        }

        for (uint32_t i = 0; i < framebuffer->attachments.size(); ++i) {
            const auto &attachment = framebuffer->attachments[i];

            if (attachment.type == opal::Attachment::Type::Color) {
                VkAttachmentReference colorRef{};
                colorRef.attachment = i;
                colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs.push_back(colorRef);
            } else if (attachment.type == opal::Attachment::Type::Depth ||
                       attachment.type ==
                           opal::Attachment::Type::DepthStencil) {
                depthAttachmentRef.attachment = i;
                depthAttachmentRef.layout =
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                hasDepthAttachment = true;
            }
        }
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount =
        static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    if (hasDepthAttachment) {
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependencies[2] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(Device::globalDevice, &renderPassInfo, nullptr,
                           &renderPass->renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto &shader : pipeline->shaderProgram->attachedShaders) {
        shaderStages.push_back(shader->makeShaderStageInfo());
    }
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &pipeline->vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &pipeline->inputAssembly;
    pipelineInfo.pViewportState = &pipeline->viewportState;
    pipelineInfo.pRasterizationState = &pipeline->rasterizer;

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    if (framebuffer->isDefaultFramebuffer) {
        sampleCount = VK_SAMPLE_COUNT_1_BIT;
    } else if (!framebuffer->attachments.empty()) {
        int samples = framebuffer->attachments[0].texture->samples;
        if (samples > 1) {
            sampleCount = VK_SAMPLE_COUNT_4_BIT;
        }
    }

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &pipeline->depthStencil;

    VkPipelineColorBlendStateCreateInfo colorBlending = pipeline->colorBlending;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments =
        pipeline->vkColorBlendAttachments;

    if (colorAttachmentRefs.empty()) {
        colorBlendAttachments.clear();
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;
    } else {
        if (colorBlendAttachments.empty()) {
            VkPipelineColorBlendAttachmentState defaultState{};
            defaultState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            defaultState.blendEnable = VK_FALSE;
            colorBlendAttachments.push_back(defaultState);
        }

        if (colorBlendAttachments.size() < colorAttachmentRefs.size()) {
            auto templateState = colorBlendAttachments.front();
            colorBlendAttachments.resize(colorAttachmentRefs.size(),
                                         templateState);
        } else if (colorBlendAttachments.size() > colorAttachmentRefs.size()) {
            colorBlendAttachments.resize(colorAttachmentRefs.size());
        }

        colorBlending.attachmentCount =
            static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlending.pAttachments = colorBlendAttachments.data();
    }

    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &pipeline->dynamicState;
    pipelineInfo.layout = pipeline->pipelineLayout;
    pipelineInfo.renderPass = renderPass->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(Device::globalDevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &renderPass->pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    RenderPass::cachedRenderPasses.push_back(renderPass);

    return renderPass;
}

std::shared_ptr<CoreRenderPass> CoreRenderPass::createWithExistingRenderPass(
    std::shared_ptr<Pipeline> pipeline,
    std::shared_ptr<Framebuffer> framebuffer, VkRenderPass existingRenderPass) {
    auto coreRenderPass = std::make_shared<CoreRenderPass>();
    coreRenderPass->opalPipeline = pipeline;
    coreRenderPass->opalFramebuffer = framebuffer;
    coreRenderPass->renderPass = existingRenderPass;

    pipeline->buildPipelineLayout();

    size_t colorAttachmentCount = 0;
    if (framebuffer->isDefaultFramebuffer) {
        colorAttachmentCount = 2; // color + bright attachment
    } else {
        for (const auto &attachment : framebuffer->attachments) {
            if (attachment.type == opal::Attachment::Type::Color) {
                colorAttachmentCount++;
            }
        }
    }

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    if (!framebuffer->isDefaultFramebuffer &&
        !framebuffer->attachments.empty()) {
        int samples = framebuffer->attachments[0].texture->samples;
        if (samples > 1) {
            sampleCount = VK_SAMPLE_COUNT_4_BIT;
        }
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto &shader : pipeline->shaderProgram->attachedShaders) {
        shaderStages.push_back(shader->makeShaderStageInfo());
    }
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &pipeline->vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &pipeline->inputAssembly;
    pipelineInfo.pViewportState = &pipeline->viewportState;
    pipelineInfo.pRasterizationState = &pipeline->rasterizer;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &pipeline->depthStencil;

    // Set up color blend attachments
    VkPipelineColorBlendStateCreateInfo colorBlending = pipeline->colorBlending;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments =
        pipeline->vkColorBlendAttachments;

    if (colorAttachmentCount == 0) {
        colorBlendAttachments.clear();
        colorBlending.attachmentCount = 0;
        colorBlending.pAttachments = nullptr;
    } else {
        if (colorBlendAttachments.empty()) {
            VkPipelineColorBlendAttachmentState defaultState{};
            defaultState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            defaultState.blendEnable = VK_FALSE;
            colorBlendAttachments.push_back(defaultState);
        }

        if (colorBlendAttachments.size() < colorAttachmentCount) {
            auto templateState = colorBlendAttachments.front();
            colorBlendAttachments.resize(colorAttachmentCount, templateState);
        } else if (colorBlendAttachments.size() > colorAttachmentCount) {
            colorBlendAttachments.resize(colorAttachmentCount);
        }

        colorBlending.attachmentCount =
            static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlending.pAttachments = colorBlendAttachments.data();
    }

    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &pipeline->dynamicState;
    pipelineInfo.layout = pipeline->pipelineLayout;
    pipelineInfo.renderPass = existingRenderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(Device::globalDevice, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &coreRenderPass->pipeline) != VK_SUCCESS) {
        throw std::runtime_error(
            "Failed to create graphics pipeline with existing render pass!");
    }

    RenderPass::cachedRenderPasses.push_back(coreRenderPass);

    return coreRenderPass;
}

} // namespace opal
#endif