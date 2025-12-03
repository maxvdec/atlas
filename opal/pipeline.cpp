//
// pipeline.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Pipeline functions for the core renderer
// Copyright (c) 2025 maxvdec
//

#include "opal/opal.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace opal {

std::shared_ptr<Pipeline> Pipeline::create() {
    auto pipeline = std::make_shared<Pipeline>();
    return pipeline;
}

#ifdef VULKAN
std::vector<std::shared_ptr<CoreRenderPass>> RenderPass::cachedRenderPasses =
    {};
#endif

void Pipeline::setShaderProgram(std::shared_ptr<ShaderProgram> program) {
    this->shaderProgram = program;
}

void Pipeline::setVertexAttributes(
    const std::vector<VertexAttribute> &attributes,
    const VertexBinding &binding) {
    this->vertexAttributes = attributes;
    this->vertexBinding = binding;
}

void Pipeline::setPrimitiveStyle(PrimitiveStyle style) {
    this->primitiveStyle = style;
}

void Pipeline::setPatchVertices(int count) { this->patchVertices = count; }

void Pipeline::setViewport(int x, int y, int width, int height) {
    this->viewportX = x;
    this->viewportY = y;
    this->viewportWidth = width;
    this->viewportHeight = height;
}

void Pipeline::setRasterizerMode(RasterizerMode mode) {
    this->rasterizerMode = mode;
}

void Pipeline::setCullMode(CullMode mode) { this->cullMode = mode; }

void Pipeline::setFrontFace(FrontFace face) { this->frontFace = face; }

void Pipeline::enableDepthTest(bool enabled) {
    this->depthTestEnabled = enabled;
}

void Pipeline::setDepthCompareOp(CompareOp op) { this->depthCompareOp = op; }

void Pipeline::enableDepthWrite(bool enabled) {
    this->depthWriteEnabled = enabled;
}

void Pipeline::enableBlending(bool enabled) { this->blendingEnabled = enabled; }

void Pipeline::setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor) {
    this->blendSrcFactor = srcFactor;
    this->blendDstFactor = dstFactor;
}

void Pipeline::setBlendEquation(BlendEquation equation) {
    this->blendEquation = equation;
}

void Pipeline::enableMultisampling(bool enabled) {
    this->multisamplingEnabled = enabled;
}

void Pipeline::enablePolygonOffset(bool enabled) {
    this->polygonOffsetEnabled = enabled;
}

void Pipeline::setPolygonOffset(float factor, float units) {
    this->polygonOffsetFactor = factor;
    this->polygonOffsetUnits = units;
}

void Pipeline::enableClipDistance(int index, bool enabled) {
    if (enabled) {
        if (std::find(this->enabledClipDistances.begin(),
                      this->enabledClipDistances.end(),
                      index) == this->enabledClipDistances.end()) {
            this->enabledClipDistances.push_back(index);
        }
    } else {
        auto it = std::find(this->enabledClipDistances.begin(),
                            this->enabledClipDistances.end(), index);
        if (it != this->enabledClipDistances.end()) {
            this->enabledClipDistances.erase(it);
        }
    }
}

uint Pipeline::getGLBlendFactor(BlendFunc func) const {
    switch (func) {
    case BlendFunc::Zero:
        return GL_ZERO;
    case BlendFunc::One:
        return GL_ONE;
    case BlendFunc::SrcColor:
        return GL_SRC_COLOR;
    case BlendFunc::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFunc::DstColor:
        return GL_DST_COLOR;
    case BlendFunc::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFunc::SrcAlpha:
        return GL_SRC_ALPHA;
    case BlendFunc::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFunc::DstAlpha:
        return GL_DST_ALPHA;
    case BlendFunc::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        return GL_ONE;
    }
}

uint Pipeline::getGLBlendEquation(BlendEquation equation) const {
    switch (equation) {
    case BlendEquation::Add:
        return GL_FUNC_ADD;
    case BlendEquation::Subtract:
        return GL_FUNC_SUBTRACT;
    case BlendEquation::ReverseSubtract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case BlendEquation::Min:
        return GL_MIN;
    case BlendEquation::Max:
        return GL_MAX;
    default:
        return GL_FUNC_ADD;
    }
}

uint Pipeline::getGLCompareOp(CompareOp op) const {
    switch (op) {
    case CompareOp::Never:
        return GL_NEVER;
    case CompareOp::Less:
        return GL_LESS;
    case CompareOp::Equal:
        return GL_EQUAL;
    case CompareOp::LessEqual:
        return GL_LEQUAL;
    case CompareOp::Greater:
        return GL_GREATER;
    case CompareOp::NotEqual:
        return GL_NOTEQUAL;
    case CompareOp::GreaterEqual:
        return GL_GEQUAL;
    case CompareOp::Always:
        return GL_ALWAYS;
    default:
        return GL_LESS;
    }
}

uint Pipeline::getGLPrimitiveStyle(PrimitiveStyle style) const {
    switch (style) {
    case PrimitiveStyle::Points:
        return GL_POINTS;
    case PrimitiveStyle::Lines:
        return GL_LINES;
    case PrimitiveStyle::LineStrip:
        return GL_LINE_STRIP;
    case PrimitiveStyle::Triangles:
        return GL_TRIANGLES;
    case PrimitiveStyle::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case PrimitiveStyle::TriangleFan:
        return GL_TRIANGLE_FAN;
    case PrimitiveStyle::Patches:
        return GL_PATCHES;
    default:
        return GL_TRIANGLES;
    }
}

uint Pipeline::getGLRasterizerMode(RasterizerMode mode) const {
    switch (mode) {
    case RasterizerMode::Fill:
        return GL_FILL;
    case RasterizerMode::Line:
        return GL_LINE;
    case RasterizerMode::Point:
        return GL_POINT;
    default:
        return GL_FILL;
    }
}

uint Pipeline::getGLCullMode(CullMode mode) const {
    switch (mode) {
    case CullMode::None:
        return 0;
    case CullMode::Front:
        return GL_FRONT;
    case CullMode::Back:
        return GL_BACK;
    case CullMode::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        return GL_BACK;
    }
}

uint Pipeline::getGLFrontFace(FrontFace face) const {
    switch (face) {
    case FrontFace::Clockwise:
        return GL_CW;
    case FrontFace::CounterClockwise:
        return GL_CCW;
    default:
        return GL_CCW;
    }
}

uint Pipeline::getGLVertexAttributeType(VertexAttributeType type) const {
    switch (type) {
    case VertexAttributeType::Float:
        return GL_FLOAT;
    case VertexAttributeType::Int:
        return GL_INT;
    case VertexAttributeType::UnsignedInt:
        return GL_UNSIGNED_INT;
    case VertexAttributeType::Short:
        return GL_SHORT;
    case VertexAttributeType::UnsignedShort:
        return GL_UNSIGNED_SHORT;
    case VertexAttributeType::Byte:
        return GL_BYTE;
    case VertexAttributeType::UnsignedByte:
        return GL_UNSIGNED_BYTE;
    default:
        return GL_FLOAT;
    }
}

void Pipeline::build() {
#ifdef OPENGL
    (void)this; // Vertex layout applied explicitly per VAO.
#elif defined(VULKAN)
    this->buildPipelineLayout();
#endif
}

void Pipeline::bind() {
#ifdef OPENGL
    if (this->shaderProgram == nullptr) {
        throw std::runtime_error(
            "Pipeline::bind() called but no shader program is set. "
            "Call setShaderProgram() or refreshPipeline() first.");
    }
    glUseProgram(this->shaderProgram->programID);

    glViewport(this->viewportX, this->viewportY, this->viewportWidth,
               this->viewportHeight);

    glPolygonMode(GL_FRONT_AND_BACK,
                  this->getGLRasterizerMode(this->rasterizerMode));

    if (this->cullMode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(this->getGLCullMode(this->cullMode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    glFrontFace(this->getGLFrontFace(this->frontFace));

    if (this->depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(this->getGLCompareOp(this->depthCompareOp));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(this->depthWriteEnabled ? GL_TRUE : GL_FALSE);

    if (this->blendingEnabled) {
        glEnable(GL_BLEND);
        glBlendFunc(this->getGLBlendFactor(this->blendSrcFactor),
                    this->getGLBlendFactor(this->blendDstFactor));
        glBlendEquation(this->getGLBlendEquation(this->blendEquation));
    } else {
        glDisable(GL_BLEND);
    }

    if (this->multisamplingEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    if (this->polygonOffsetEnabled) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(this->polygonOffsetFactor, this->polygonOffsetUnits);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // Handle clip distances (up to 8 supported)
    for (int i = 0; i < 8; ++i) {
        bool shouldEnable = std::find(this->enabledClipDistances.begin(),
                                      this->enabledClipDistances.end(),
                                      i) != this->enabledClipDistances.end();
        if (shouldEnable) {
            glEnable(GL_CLIP_DISTANCE0 + i);
        } else {
            glDisable(GL_CLIP_DISTANCE0 + i);
        }
    }
#endif
}

bool Pipeline::operator==(std::shared_ptr<Pipeline> pipeline) const {
    if (this->primitiveStyle != pipeline->primitiveStyle) {
        return false;
    }
    if (this->rasterizerMode != pipeline->rasterizerMode) {
        return false;
    }
    if (this->cullMode != pipeline->cullMode) {
        return false;
    }
    if (this->frontFace != pipeline->frontFace) {
        return false;
    }
    if (this->blendingEnabled != pipeline->blendingEnabled) {
        return false;
    }
    if (this->blendSrcFactor != pipeline->blendSrcFactor) {
        return false;
    }
    if (this->blendDstFactor != pipeline->blendDstFactor) {
        return false;
    }
    if (this->depthTestEnabled != pipeline->depthTestEnabled) {
        return false;
    }
    if (this->depthCompareOp != pipeline->depthCompareOp) {
        return false;
    }
    if (this->shaderProgram != pipeline->shaderProgram) {
        return false;
    }
    if (this->vertexAttributes != pipeline->vertexAttributes) {
        return false;
    }
    if (this->vertexBinding.inputRate != pipeline->vertexBinding.inputRate) {
        return false;
    }
    if (this->vertexBinding.stride != pipeline->vertexBinding.stride) {
        return false;
    }
    return true;
}

void Pipeline::setUniform1f(const std::string &name, float v0) {
#ifdef OPENGL
    glUniform1f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        // Uniform not found, silently ignore for compatibility
        return;
    }
    if (!info->isBuffer) {
        // Push constant
        updatePushConstant(info->offset, &v0, sizeof(float));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &v0,
                          sizeof(float));
    }
#endif
}

void Pipeline::setUniformMat4f(const std::string &name,
                               const glm::mat4 &matrix) {
#ifdef OPENGL
    glUniformMatrix4fv(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), 1,
        GL_FALSE, &matrix[0][0]);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &matrix[0][0], sizeof(glm::mat4));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &matrix[0][0],
                          sizeof(glm::mat4));
    }
#endif
}

void Pipeline::setUniform3f(const std::string &name, float v0, float v1,
                            float v2) {
#ifdef OPENGL
    glUniform3f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    float data[3] = {v0, v1, v2};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#endif
}

void Pipeline::setUniform1i(const std::string &name, int v0) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &v0, sizeof(int));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &v0,
                          sizeof(int));
    }
#endif
}

void Pipeline::setUniformBool(const std::string &name, bool value) {
#ifdef OPENGL
    glUniform1i(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()),
        (int)value);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    int intValue = value ? 1 : 0;
    if (!info->isBuffer) {
        updatePushConstant(info->offset, &intValue, sizeof(int));
    } else {
        updateUniformData(info->set, info->binding, info->offset, &intValue,
                          sizeof(int));
    }
#endif
}

void Pipeline::setUniform4f(const std::string &name, float v0, float v1,
                            float v2, float v3) {
#ifdef OPENGL
    glUniform4f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1, v2, v3);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    float data[4] = {v0, v1, v2, v3};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#endif
}

void Pipeline::setUniform2f(const std::string &name, float v0, float v1) {
#ifdef OPENGL
    glUniform2f(
        glGetUniformLocation(this->shaderProgram->programID, name.c_str()), v0,
        v1);
#elif defined(VULKAN)
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        return;
    }
    float data[2] = {v0, v1};
    if (!info->isBuffer) {
        updatePushConstant(info->offset, data, sizeof(data));
    } else {
        updateUniformData(info->set, info->binding, info->offset, data,
                          sizeof(data));
    }
#endif
}

void Pipeline::bindBufferData(const std::string &name, const void *data,
                              size_t size) {
    if (!shaderProgram || !data || size == 0) {
        return;
    }

#ifdef OPENGL
    // In OpenGL, we don't need to do anything special for UBOs
    // The data will be uploaded through the uniform buffer object system
    // For now, this is a no-op on OpenGL as the individual uniform setters
    // handle the data. If you need full UBO support, you'd use
    // glBufferSubData here.
    (void)name;
    (void)data;
    (void)size;
#elif defined(VULKAN)
    // Find the uniform buffer binding info
    const UniformBindingInfo *info = this->shaderProgram->findUniform(name);
    if (!info) {
        // Try to find it as just the buffer name (without member access)
        return;
    }

    if (!info->isBuffer) {
        // This is a push constant, not a buffer
        return;
    }

    // Update the entire buffer
    updateUniformData(info->set, info->binding, 0, data, size);
#endif
}

#ifdef VULKAN
Pipeline::UniformBufferAllocation &
Pipeline::getOrCreateUniformBuffer(uint32_t set, uint32_t binding,
                                   VkDeviceSize size) {

    const DescriptorBindingInfoEntry *bindingInfo =
        getDescriptorBindingInfo(set, binding);
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (bindingInfo) {
        descriptorType = bindingInfo->type;
    }
    VkBufferUsageFlags usage =
        descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            : VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    uint64_t key = makeBindingKey(set, binding);
    auto it = uniformBuffers.find(key);

    if (it != uniformBuffers.end()) {
        // Buffer exists, check if size is sufficient
        if (it->second.descriptorType == descriptorType &&
            it->second.size >= size) {
            return it->second;
        }
        // Need to recreate with larger size
        if (it->second.mappedData) {
            vkUnmapMemory(Device::globalDevice, it->second.memory);
        }
        vkDestroyBuffer(Device::globalDevice, it->second.buffer, nullptr);
        vkFreeMemory(Device::globalDevice, it->second.memory, nullptr);
    }

    // Create new buffer
    UniformBufferAllocation &alloc = uniformBuffers[key];
    alloc.size = size;
    alloc.descriptorType = descriptorType;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device::globalDevice, &bufferInfo, nullptr,
                       &alloc.buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create uniform buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device::globalDevice, alloc.buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Device::globalInstance->findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(Device::globalDevice, &allocInfo, nullptr,
                         &alloc.memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate uniform buffer memory");
    }

    vkBindBufferMemory(Device::globalDevice, alloc.buffer, alloc.memory, 0);

    // Map the memory persistently
    vkMapMemory(Device::globalDevice, alloc.memory, 0, size, 0,
                &alloc.mappedData);

    // Initialize to zero
    memset(alloc.mappedData, 0, size);

    return alloc;
}

const Pipeline::DescriptorBindingInfoEntry *
Pipeline::getDescriptorBindingInfo(uint32_t set, uint32_t binding) const {
    auto setIt = descriptorBindingInfo.find(set);
    if (setIt == descriptorBindingInfo.end()) {
        return nullptr;
    }
    auto bindingIt = setIt->second.find(binding);
    if (bindingIt == setIt->second.end()) {
        return nullptr;
    }
    return &bindingIt->second;
}

void Pipeline::updateUniformData(uint32_t set, uint32_t binding,
                                 uint32_t offset, const void *data,
                                 size_t size) {
    // Find the block size from reflection data
    VkDeviceSize blockSize = 256; // Default minimum uniform buffer alignment

    // Look through shader program's uniform bindings to find the block size
    if (this->shaderProgram) {
        for (const auto &pair : this->shaderProgram->uniformBindings) {
            const UniformBindingInfo &info = pair.second;
            if (info.set == set && info.binding == binding && info.isBuffer &&
                info.size > 0) {
                if (info.offset == 0) {
                    // This is the block itself, use its size
                    blockSize = info.size;
                    break;
                }
            }
        }
    }

    // Ensure we allocate at least enough for this write
    VkDeviceSize requiredSize = offset + size;
    if (blockSize < requiredSize) {
        blockSize = requiredSize;
    }

    UniformBufferAllocation &alloc =
        getOrCreateUniformBuffer(set, binding, blockSize);

    // Write data at the correct offset
    char *dst = static_cast<char *>(alloc.mappedData) + offset;
    memcpy(dst, data, size);

    bindUniformBufferDescriptor(set, binding);
}

void Pipeline::buildDescriptorSets() {
    // This function would build descriptor sets based on reflection data
    // For now, we rely on the existing descriptor set management in the render
    // pass A full implementation would:
    // 1. Create descriptor set layouts based on uniform bindings
    // 2. Allocate descriptor sets from a pool
    // 3. Update descriptor sets with buffer/image bindings
}

void Pipeline::resetDescriptorSets() {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(Device::globalDevice, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    descriptorSets.clear();
}

void Pipeline::ensureDescriptorResources() {
    if (descriptorBindingInfo.empty()) {
        return;
    }

    // Check if we already have valid descriptor sets
    // If descriptorPool exists and has allocated sets, don't reallocate
    if (descriptorPool != VK_NULL_HANDLE && !descriptorSets.empty()) {
        bool allSetsValid = true;
        for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
            if (descriptorSetLayouts[i] != VK_NULL_HANDLE &&
                (i >= descriptorSets.size() ||
                 descriptorSets[i] == VK_NULL_HANDLE)) {
                allSetsValid = false;
                break;
            }
        }
        if (allSetsValid) {
            return; // Already have valid descriptor resources
        }
    }

    // Only allocate if we don't have a pool yet
    if (descriptorPool != VK_NULL_HANDLE) {
        // Pool exists but sets are invalid - this shouldn't happen during
        // normal operation. Log a warning but don't reallocate during a frame.
        return;
    }

    std::unordered_map<VkDescriptorType, uint32_t> typeCounts;
    uint32_t setCount = 0;
    for (const auto &setPair : descriptorBindingInfo) {
        uint32_t setIdx = setPair.first;
        if (setIdx >= descriptorSetLayouts.size() ||
            descriptorSetLayouts[setIdx] == VK_NULL_HANDLE) {
            continue;
        }
        setCount++;
        for (const auto &bindingPair : setPair.second) {
            typeCounts[bindingPair.second.type] += bindingPair.second.count;
        }
    }

    if (setCount == 0 || typeCounts.empty()) {
        return;
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(typeCounts.size());
    for (const auto &pair : typeCounts) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = pair.first;
        poolSize.descriptorCount = pair.second;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = setCount;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    if (vkCreateDescriptorPool(Device::globalDevice, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    descriptorSets.assign(descriptorSetLayouts.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
        if (descriptorSetLayouts[i] == VK_NULL_HANDLE) {
            continue;
        }

        VkDescriptorSetLayout layout = descriptorSetLayouts[i];
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet set = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(Device::globalDevice, &allocInfo, &set) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }
        descriptorSets[i] = set;
    }

    // Prime all descriptors with placeholder resources so Vulkan validation
    // layers always see valid bindings even before the app uploads data.
    auto dummyTex = getDummyTexture();
    auto dummyCubeTex = getDummyCubemap();
    for (const auto &setPair : descriptorBindingInfo) {
        uint32_t setIndex = setPair.first;
        if (setIndex >= descriptorSets.size() ||
            descriptorSets[setIndex] == VK_NULL_HANDLE) {
            continue;
        }
        for (const auto &bindingPair : setPair.second) {
            if (bindingPair.second.isBuffer) {
                bindUniformBufferDescriptor(setIndex, bindingPair.first);
            } else if (bindingPair.second.isSampler) {
                if (bindingPair.second.isCubemap) {
                    bindSamplerDescriptor(setIndex, bindingPair.first,
                                          dummyCubeTex);
                } else {
                    bindSamplerDescriptor(setIndex, bindingPair.first,
                                          dummyTex);
                }
            }
        }
    }
}

std::shared_ptr<Texture> Pipeline::getDummyTexture() {
    static std::shared_ptr<Texture> dummy = nullptr;
    if (!dummy) {
        uint8_t white[4] = {255, 255, 255, 255};
        dummy = Texture::create(TextureType::Texture2D, TextureFormat::Rgba8, 1,
                                1, TextureDataFormat::Rgba, white, 1);
    }
    return dummy;
}

std::shared_ptr<Texture> Pipeline::getDummyCubemap() {
    static std::shared_ptr<Texture> dummyCube = nullptr;
    if (!dummyCube) {
        // Create a 1x1 cubemap without initial data - it will be uninitialized
        // but that's fine for a placeholder texture. We avoid passing data
        // because the staging buffer size calculation in Texture::create
        // doesn't account for cubemap layers.
        dummyCube =
            Texture::create(TextureType::TextureCubeMap, TextureFormat::Rgba8,
                            1, 1, TextureDataFormat::Rgba, nullptr, 1);
    }
    return dummyCube;
}

void Pipeline::bindSamplerDescriptor(uint32_t set, uint32_t binding,
                                     std::shared_ptr<Texture> texture) {
    if (!texture) {
        return;
    }

    ensureDescriptorResources();

    if (set >= descriptorSets.size() || descriptorSets[set] == VK_NULL_HANDLE) {
        return;
    }

    const auto *info = getDescriptorBindingInfo(set, binding);
    if (info == nullptr || !info->isSampler) {
        return;
    }

    // Ensure texture is in shader read layout before binding
    VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (texture->currentLayout != desiredLayout &&
        texture->currentLayout != VK_IMAGE_LAYOUT_GENERAL) {
        // Only transition color textures to shader read
        // Depth textures may need to stay in attachment layout
        VkFormat vkFormat = opalTextureFormatToVulkanFormat(texture->format);
        bool isDepth = (texture->format == TextureFormat::Depth24Stencil8 ||
                        texture->format == TextureFormat::DepthComponent24 ||
                        texture->format == TextureFormat::Depth32F);

        if (!isDepth || texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            uint32_t layerCount =
                (texture->type == TextureType::TextureCubeMap) ? 6 : 1;
            Framebuffer::transitionImageLayout(texture->vkImage, vkFormat,
                                               texture->currentLayout,
                                               desiredLayout, layerCount);
            texture->currentLayout = desiredLayout;
        }
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture->vkSampler;
    imageInfo.imageView = texture->vkImageView;
    imageInfo.imageLayout =
        (texture->currentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : texture->currentLayout;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[set];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(Device::globalDevice, 1, &write, 0, nullptr);
}

void Pipeline::bindUniformBufferDescriptor(uint32_t set, uint32_t binding) {
    if (descriptorBindingInfo.empty()) {
        return;
    }

    ensureDescriptorResources();

    if (set >= descriptorSets.size() || descriptorSets[set] == VK_NULL_HANDLE) {
        return;
    }

    const auto *info = getDescriptorBindingInfo(set, binding);
    if (info == nullptr) {
        return;
    }
    if (!info->isBuffer) {
        return;
    }

    VkDeviceSize minSize = info->minBufferSize > 0 ? info->minBufferSize : 256;
    UniformBufferAllocation &alloc =
        getOrCreateUniformBuffer(set, binding, minSize);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = alloc.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = alloc.size;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSets[set];
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = info->type;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(Device::globalDevice, 1, &write, 0, nullptr);
}

void Pipeline::bindDescriptorSets(VkCommandBuffer commandBuffer) {
    if (descriptorSetLayouts.empty()) {
        return;
    }

    ensureDescriptorResources();

    uint32_t currentStart = UINT32_MAX;
    std::vector<VkDescriptorSet> run;

    for (uint32_t i = 0; i < descriptorSets.size(); ++i) {
        bool setValid = i < descriptorSetLayouts.size() &&
                        descriptorSetLayouts[i] != VK_NULL_HANDLE &&
                        descriptorSets[i] != VK_NULL_HANDLE;

        if (!setValid) {
            if (!run.empty() && currentStart != UINT32_MAX) {
                vkCmdBindDescriptorSets(
                    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, currentStart,
                    static_cast<uint32_t>(run.size()), run.data(), 0, nullptr);
                run.clear();
                currentStart = UINT32_MAX;
            }
            continue;
        }

        if (currentStart == UINT32_MAX) {
            currentStart = i;
        }
        run.push_back(descriptorSets[i]);
    }

    if (!run.empty() && currentStart != UINT32_MAX) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, currentStart,
                                static_cast<uint32_t>(run.size()), run.data(),
                                0, nullptr);
    }
}

void Pipeline::updatePushConstant(uint32_t offset, const void *data,
                                  size_t size) {
    // Ensure push constant buffer is large enough
    uint32_t requiredSize = offset + static_cast<uint32_t>(size);
    if (pushConstantData.size() < requiredSize) {
        pushConstantData.resize(requiredSize, 0);
        pushConstantSize = requiredSize;
    }

    // Copy data to push constant buffer
    memcpy(pushConstantData.data() + offset, data, size);
    pushConstantsDirty = true;
}

void Pipeline::flushPushConstants(VkCommandBuffer commandBuffer) {
    // Push constants must be set before drawing if the shader uses them.
    // We push even if not dirty to ensure the GPU has valid data on first use.
    if (pushConstantSize == 0) {
        return;
    }

    vkCmdPushConstants(commandBuffer, pipelineLayout, pushConstantStages, 0,
                       pushConstantSize, pushConstantData.data());

    pushConstantsDirty = false;
}
#endif

} // namespace opal
