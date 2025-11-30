//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for Vulkan
// Copyright (c) 2025 Max Van den Eynde
//

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
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create vertex input state

    vertexInputInfo = {};
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = vertexBinding.stride;
    bindingDesc.inputRate =
        (vertexBinding.inputRate == VertexBindingInputRate::Vertex)
            ? VK_VERTEX_INPUT_RATE_VERTEX
            : VK_VERTEX_INPUT_RATE_INSTANCE;

    bindingDescriptions.push_back(bindingDesc);

    for (const auto &attr : this->vertexAttributes) {
        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.location = attr.location;
        attrDesc.binding = 0;
        attrDesc.format =
            this->getFormat(attr.type, attr.size, attr.normalized);
        attrDesc.offset = attr.offset;

        attributeDescriptions.push_back(attrDesc);
    }

    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount =
        static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Define input assembly state

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

    // Create the viewport
    VkViewport viewport{};
    viewport.x = static_cast<float>(viewportX);
    viewport.y = static_cast<float>(viewportY);
    viewport.width = static_cast<float>(viewportWidth);
    viewport.height = static_cast<float>(viewportHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = {.width = static_cast<uint32_t>(viewportWidth),
                      .height = static_cast<uint32_t>(viewportHeight)};

    viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Create the rasterizer state
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

    // Depth and stencil testing
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

    // Create the color blend state
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

    colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(Device::globalDevice, &pipelineLayoutInfo,
                               nullptr, &this->pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
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
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        } else if (attachment.type == opal::Attachment::Type::Depth ||
                   attachment.type == opal::Attachment::Type::DepthStencil) {
            attachmentDesc.finalLayout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        attachments.push_back(attachmentDesc);
    }

    std::vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentReference depthAttachmentRef{};
    bool hasDepthAttachment = false;

    for (uint32_t i = 0; i < framebuffer->attachments.size(); ++i) {
        const auto &attachment = framebuffer->attachments[i];

        if (attachment.type == opal::Attachment::Type::Color) {
            VkAttachmentReference colorRef{};
            colorRef.attachment = i;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.push_back(colorRef);
        } else if (attachment.type == opal::Attachment::Type::Depth ||
                   attachment.type == opal::Attachment::Type::DepthStencil) {
            depthAttachmentRef.attachment = i;
            depthAttachmentRef.layout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            hasDepthAttachment = true;
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
    if (pipeline->multisamplingEnabled) {
        pipelineInfo.pMultisampleState = &pipeline->multisampling;
    } else {
        pipelineInfo.pMultisampleState = nullptr;
    }
    pipelineInfo.pDepthStencilState = &pipeline->depthStencil;
    pipelineInfo.pColorBlendState = &pipeline->colorBlending;
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

} // namespace opal
#endif