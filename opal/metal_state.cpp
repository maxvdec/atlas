#ifdef METAL

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>

#include "metal_state.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace opal::metal {

namespace {

template <typename T>
static inline T alignUp(T value, T alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

static std::string trim(const std::string &input) {
    size_t start = 0;
    while (start < input.size() &&
           std::isspace(static_cast<unsigned char>(input[start]))) {
        start++;
    }
    size_t end = input.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        end--;
    }
    return input.substr(start, end - start);
}

static std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : input) {
        if (c == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

struct RawField {
    std::string typeName;
    std::string fieldName;
    size_t arrayCount = 1;
};

struct RawStruct {
    std::string name;
    std::vector<RawField> fields;
};

static bool parseTemplateArray(const std::string &typeName,
                               std::string &innerType, size_t &count) {
    static const std::regex templateRegex(
        R"(spvUnsafeArray\s*<\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*(\d+)\s*>)");
    std::smatch match;
    if (!std::regex_match(typeName, match, templateRegex)) {
        return false;
    }
    innerType = match[1].str();
    count = static_cast<size_t>(std::stoul(match[2].str()));
    return true;
}

static std::unordered_map<std::string, RawStruct>
parseRawStructs(const std::string &source) {
    std::unordered_map<std::string, RawStruct> structs;

    static const std::regex structStart(
        R"(struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{)");
    auto begin = std::sregex_iterator(source.begin(), source.end(), structStart);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const std::string structName = (*it)[1].str();
        size_t bodyStart = static_cast<size_t>((*it).position(0) +
                                               (*it).length(0));
        int depth = 1;
        size_t cursor = bodyStart;
        while (cursor < source.size() && depth > 0) {
            if (source[cursor] == '{') {
                depth++;
            } else if (source[cursor] == '}') {
                depth--;
            }
            cursor++;
        }
        if (depth != 0 || cursor <= bodyStart) {
            continue;
        }

        const std::string body =
            source.substr(bodyStart, (cursor - 1) - bodyStart);
        RawStruct raw;
        raw.name = structName;

        std::vector<std::string> statements = split(body, ';');
        for (std::string statement : statements) {
            size_t commentPos = statement.find("//");
            if (commentPos != std::string::npos) {
                statement = statement.substr(0, commentPos);
            }
            statement = trim(statement);
            if (statement.empty()) {
                continue;
            }

            size_t lastSpace = statement.find_last_of(" \t\r\n");
            if (lastSpace == std::string::npos) {
                continue;
            }

            std::string typeName = trim(statement.substr(0, lastSpace));
            std::string declarator = trim(statement.substr(lastSpace + 1));
            if (typeName.empty() || declarator.empty()) {
                continue;
            }

            static const std::regex declRegex(
                R"(([A-Za-z_][A-Za-z0-9_]*)(?:\s*\[\s*(\d+)\s*\])?)");
            std::smatch declMatch;
            if (!std::regex_match(declarator, declMatch, declRegex)) {
                continue;
            }

            RawField field;
            field.fieldName = declMatch[1].str();
            field.arrayCount =
                declMatch[2].matched
                    ? static_cast<size_t>(std::stoul(declMatch[2].str()))
                    : 1;

            std::string innerType;
            size_t templateCount = 1;
            if (parseTemplateArray(typeName, innerType, templateCount)) {
                field.typeName = innerType;
                if (field.arrayCount == 1) {
                    field.arrayCount = templateCount;
                } else {
                    field.arrayCount *= templateCount;
                }
            } else {
                field.typeName = typeName;
            }

            raw.fields.push_back(field);
        }

        if (!raw.fields.empty()) {
            structs[structName] = raw;
        }
    }

    return structs;
}

static std::optional<FieldType> builtinType(const std::string &name) {
    if (name == "float" || name == "int" || name == "uint" || name == "bool") {
        return FieldType{4, 4, false, ""};
    }
    if (name == "half") {
        return FieldType{2, 2, false, ""};
    }

    static const std::regex vecRegex(
        R"((packed_)?(float|int|uint|half)([2-4]))");
    std::smatch vecMatch;
    if (std::regex_match(name, vecMatch, vecRegex)) {
        bool packed = vecMatch[1].matched;
        std::string scalar = vecMatch[2].str();
        int count = std::stoi(vecMatch[3].str());

        size_t scalarSize = 4;
        if (scalar == "half") {
            scalarSize = 2;
        }

        if (packed) {
            return FieldType{scalarSize * static_cast<size_t>(count), scalarSize,
                             false, ""};
        }

        if (count == 2) {
            return FieldType{scalarSize * 2, scalarSize * 2, false, ""};
        }
        if (count == 3) {
            return FieldType{scalarSize * 4, scalarSize * 4, false, ""};
        }
        return FieldType{scalarSize * 4, scalarSize * 4, false, ""};
    }

    static const std::regex matrixRegex(R"((float|half)([2-4])x([2-4]))");
    std::smatch matrixMatch;
    if (std::regex_match(name, matrixMatch, matrixRegex)) {
        size_t scalarSize = matrixMatch[1].str() == "half" ? 2 : 4;
        int columns = std::stoi(matrixMatch[2].str());
        int rows = std::stoi(matrixMatch[3].str());

        size_t vectorAlignment =
            (rows == 2) ? scalarSize * 2 : scalarSize * 4;
        size_t vectorSize = (rows == 3) ? scalarSize * 4 : scalarSize * rows;
        vectorSize = alignUp(vectorSize, vectorAlignment);
        size_t matrixSize = vectorSize * static_cast<size_t>(columns);
        return FieldType{matrixSize, vectorAlignment, false, ""};
    }

    return std::nullopt;
}

static StructLayout computeLayout(
    const std::string &name,
    const std::unordered_map<std::string, RawStruct> &rawStructs,
    std::unordered_map<std::string, StructLayout> &cache,
    std::unordered_set<std::string> &visiting);

static size_t findMatchingParen(const std::string &source, size_t openParenPos) {
    if (openParenPos >= source.size() || source[openParenPos] != '(') {
        return std::string::npos;
    }
    int depth = 0;
    for (size_t i = openParenPos; i < source.size(); ++i) {
        char c = source[i];
        if (c == '(') {
            depth++;
        } else if (c == ')') {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::string::npos;
}

static FieldType resolveType(
    const std::string &typeName,
    const std::unordered_map<std::string, RawStruct> &rawStructs,
    std::unordered_map<std::string, StructLayout> &cache,
    std::unordered_set<std::string> &visiting) {
    auto builtin = builtinType(typeName);
    if (builtin.has_value()) {
        return builtin.value();
    }

    auto rawIt = rawStructs.find(typeName);
    if (rawIt != rawStructs.end()) {
        StructLayout nested = computeLayout(typeName, rawStructs, cache, visiting);
        return FieldType{nested.size, nested.alignment, true, typeName};
    }

    return FieldType{};
}

static StructLayout computeLayout(
    const std::string &name,
    const std::unordered_map<std::string, RawStruct> &rawStructs,
    std::unordered_map<std::string, StructLayout> &cache,
    std::unordered_set<std::string> &visiting) {
    auto cacheIt = cache.find(name);
    if (cacheIt != cache.end()) {
        return cacheIt->second;
    }

    if (visiting.find(name) != visiting.end()) {
        return StructLayout{};
    }

    auto rawIt = rawStructs.find(name);
    if (rawIt == rawStructs.end()) {
        return StructLayout{};
    }

    visiting.insert(name);
    StructLayout layout;
    layout.name = name;
    layout.alignment = 1;
    size_t cursor = 0;

    for (const RawField &rawField : rawIt->second.fields) {
        FieldType fieldType =
            resolveType(rawField.typeName, rawStructs, cache, visiting);
        if (fieldType.size == 0) {
            continue;
        }

        cursor = alignUp(cursor, fieldType.alignment);

        StructField field;
        field.name = rawField.fieldName;
        field.type = fieldType;
        field.offset = cursor;
        field.arrayCount = std::max<size_t>(1, rawField.arrayCount);
        field.stride = alignUp(fieldType.size, fieldType.alignment);

        size_t fieldSize = (field.arrayCount > 1) ? field.stride * field.arrayCount
                                                  : fieldType.size;
        cursor += fieldSize;
        layout.alignment = std::max(layout.alignment, fieldType.alignment);
        layout.fields.push_back(field);
    }

    layout.size = alignUp(cursor, layout.alignment);
    visiting.erase(name);

    cache[name] = layout;
    return layout;
}

static std::vector<BufferBinding> parseStageBufferBindings(const std::string &source,
                                                           bool isVertexStage) {
    std::vector<BufferBinding> bindings;
    const std::regex stageStartRegex(
        isVertexStage ? R"(\bvertex\b[^\(\{;]*\()"
                      : R"(\bfragment\b[^\(\{;]*\()");
    const std::regex bufferRegex(
        R"((?:constant|device)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\*|&)\s*([A-Za-z_][A-Za-z0-9_]*)\s*\[\[buffer\((\d+)\)\]\])");

    auto stageBegin =
        std::sregex_iterator(source.begin(), source.end(), stageStartRegex);
    auto stageEnd = std::sregex_iterator();
    for (auto stageIt = stageBegin; stageIt != stageEnd; ++stageIt) {
        size_t openParenPos = static_cast<size_t>((*stageIt).position(0) +
                                                  (*stageIt).length(0) - 1);
        size_t closeParenPos = findMatchingParen(source, openParenPos);
        if (closeParenPos == std::string::npos || closeParenPos <= openParenPos) {
            continue;
        }
        const std::string params =
            source.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
        auto begin = std::sregex_iterator(params.begin(), params.end(), bufferRegex);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            BufferBinding binding;
            binding.structName = (*it)[1].str();
            binding.instanceName = (*it)[2].str();
            binding.index = static_cast<uint32_t>(std::stoul((*it)[3].str()));
            binding.vertexStage = isVertexStage;
            binding.fragmentStage = !isVertexStage;
            bindings.push_back(binding);
        }
    }

    return bindings;
}

static std::vector<std::string> parseUniformPath(const std::string &name) {
    return split(name, '.');
}

static bool parseUniformToken(const std::string &token, std::string &fieldName,
                              bool &hasIndex, size_t &index) {
    static const std::regex tokenRegex(
        R"(([A-Za-z_][A-Za-z0-9_]*)(?:\s*\[\s*(\d+)\s*\])?)");
    std::smatch match;
    if (!std::regex_match(token, match, tokenRegex)) {
        return false;
    }
    fieldName = match[1].str();
    hasIndex = match[2].matched;
    index = hasIndex ? static_cast<size_t>(std::stoul(match[2].str())) : 0;
    return true;
}

static bool hasSuffixIgnoreCase(const std::string &value,
                                const std::string &suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    size_t offset = value.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        char lhs = static_cast<char>(
            std::toupper(static_cast<unsigned char>(value[offset + i])));
        char rhs = static_cast<char>(
            std::toupper(static_cast<unsigned char>(suffix[i])));
        if (lhs != rhs) {
            return false;
        }
    }
    return true;
}

static bool matchesBindingAlias(const BufferBinding &binding,
                                const std::string &token) {
    if (!binding.instanceName.empty() && token == binding.instanceName) {
        return true;
    }
    if (token == binding.structName) {
        return true;
    }
    static const std::string suffixes[] = {"UBO", "SSBO", "BUFFER"};
    for (const std::string &suffix : suffixes) {
        if (!hasSuffixIgnoreCase(binding.structName, suffix)) {
            continue;
        }
        std::string trimmed =
            binding.structName.substr(0, binding.structName.size() - suffix.size());
        if (token == trimmed) {
            return true;
        }
    }
    return false;
}

static const StructField *findField(const StructLayout &layout,
                                    const std::string &name) {
    for (const StructField &field : layout.fields) {
        if (field.name == name) {
            return &field;
        }
    }
    return nullptr;
}

template <typename T>
static inline T enumOr(T lhs, T rhs) {
    return static_cast<T>(static_cast<NS::UInteger>(lhs) |
                          static_cast<NS::UInteger>(rhs));
}

static std::unordered_map<Context *, ContextState> &contextStatesStorage() {
    static auto *states = new std::unordered_map<Context *, ContextState>();
    return *states;
}

static std::unordered_map<Device *, DeviceState> &deviceStatesStorage() {
    static auto *states = new std::unordered_map<Device *, DeviceState>();
    return *states;
}

static std::unordered_map<Buffer *, BufferState> &bufferStatesStorage() {
    static auto *states = new std::unordered_map<Buffer *, BufferState>();
    return *states;
}

static std::unordered_map<Texture *, TextureState> &textureStatesStorage() {
    static auto *states = new std::unordered_map<Texture *, TextureState>();
    return *states;
}

static std::unordered_map<Shader *, ShaderState> &shaderStatesStorage() {
    static auto *states = new std::unordered_map<Shader *, ShaderState>();
    return *states;
}

static std::unordered_map<ShaderProgram *, ProgramState> &
programStatesStorage() {
    static auto *states =
        new std::unordered_map<ShaderProgram *, ProgramState>();
    return *states;
}

static std::unordered_map<Pipeline *, PipelineState> &pipelineStatesStorage() {
    static auto *states = new std::unordered_map<Pipeline *, PipelineState>();
    return *states;
}

static std::unordered_map<Framebuffer *, FramebufferState> &
framebufferStatesStorage() {
    static auto *states =
        new std::unordered_map<Framebuffer *, FramebufferState>();
    return *states;
}

static std::unordered_map<CommandBuffer *, CommandBufferState> &
commandStatesStorage() {
    static auto *states =
        new std::unordered_map<CommandBuffer *, CommandBufferState>();
    return *states;
}

static std::unordered_map<uint32_t, std::weak_ptr<Texture>> &
textureRegistryStorage() {
    static auto *registry =
        new std::unordered_map<uint32_t, std::weak_ptr<Texture>>();
    return *registry;
}

static uint32_t &nextTextureHandleStorage() {
    static auto *nextHandle = new uint32_t(1);
    return *nextHandle;
}

} // namespace

ContextState &contextState(Context *context) {
    return contextStatesStorage()[context];
}

DeviceState &deviceState(Device *device) { return deviceStatesStorage()[device]; }

BufferState &bufferState(Buffer *buffer) { return bufferStatesStorage()[buffer]; }

TextureState &textureState(Texture *texture) {
    return textureStatesStorage()[texture];
}

ShaderState &shaderState(Shader *shader) { return shaderStatesStorage()[shader]; }

ProgramState &programState(ShaderProgram *program) {
    return programStatesStorage()[program];
}

PipelineState &pipelineState(Pipeline *pipeline) {
    return pipelineStatesStorage()[pipeline];
}

FramebufferState &framebufferState(Framebuffer *framebuffer) {
    return framebufferStatesStorage()[framebuffer];
}

CommandBufferState &commandBufferState(CommandBuffer *commandBuffer) {
    return commandStatesStorage()[commandBuffer];
}

void releaseContextState(Context *context) {
    if (context == nullptr) {
        return;
    }
    auto &states = contextStatesStorage();
    auto it = states.find(context);
    if (it == states.end()) {
        return;
    }
    it->second.layer = nullptr;
    states.erase(it);
}

void releaseDeviceState(Device *device) {
    if (device == nullptr) {
        return;
    }
    auto &states = deviceStatesStorage();
    auto it = states.find(device);
    if (it == states.end()) {
        return;
    }
    DeviceState &state = it->second;
    state.brightTexture.reset();
    state.depthTexture.reset();
    state.drawable = nullptr;
    state.context = nullptr;
    state.drawableWidth = 0;
    state.drawableHeight = 0;
    if (state.queue != nullptr) {
        state.queue->release();
        state.queue = nullptr;
    }
    if (state.device != nullptr) {
        state.device->release();
        state.device = nullptr;
    }
    states.erase(it);
}

void releaseBufferState(Buffer *buffer) {
    if (buffer == nullptr) {
        return;
    }
    auto &states = bufferStatesStorage();
    auto it = states.find(buffer);
    if (it == states.end()) {
        return;
    }
    BufferState &state = it->second;
    if (state.buffer != nullptr) {
        state.buffer->release();
        state.buffer = nullptr;
    }
    state.size = 0;
    states.erase(it);
}

void releaseTextureState(Texture *texture) {
    if (texture == nullptr) {
        return;
    }
    auto &states = textureStatesStorage();
    auto it = states.find(texture);
    if (it == states.end()) {
        return;
    }
    TextureState &state = it->second;
    if (state.sampler != nullptr) {
        state.sampler->release();
        state.sampler = nullptr;
    }
    if (state.texture != nullptr) {
        state.texture->release();
        state.texture = nullptr;
    }
    if (state.handle != 0) {
        textureRegistryStorage().erase(state.handle);
        state.handle = 0;
    }
    states.erase(it);
}

void releaseShaderState(Shader *shader) {
    if (shader == nullptr) {
        return;
    }
    auto &states = shaderStatesStorage();
    auto it = states.find(shader);
    if (it == states.end()) {
        return;
    }
    ShaderState &state = it->second;
    if (state.function != nullptr) {
        state.function->release();
        state.function = nullptr;
    }
    if (state.library != nullptr) {
        state.library->release();
        state.library = nullptr;
    }
    states.erase(it);
}

void releaseProgramState(ShaderProgram *program) {
    if (program == nullptr) {
        return;
    }
    auto &states = programStatesStorage();
    auto it = states.find(program);
    if (it == states.end()) {
        return;
    }
    ProgramState &state = it->second;
    state.vertexFunction = nullptr;
    state.fragmentFunction = nullptr;
    state.layouts.clear();
    state.bindings.clear();
    state.bindingSize.clear();
    state.uniformResolutionCache.clear();
    states.erase(it);
}

void releasePipelineState(Pipeline *pipeline) {
    if (pipeline == nullptr) {
        return;
    }
    auto &states = pipelineStatesStorage();
    auto it = states.find(pipeline);
    if (it == states.end()) {
        return;
    }
    PipelineState &state = it->second;
    for (auto &cacheEntry : state.renderPipelineCache) {
        if (cacheEntry.second != nullptr) {
            cacheEntry.second->release();
        }
    }
    state.renderPipelineCache.clear();
    for (auto &bufferEntry : state.uniformBuffers) {
        if (bufferEntry.second != nullptr) {
            bufferEntry.second->release();
        }
    }
    state.uniformBuffers.clear();
    state.uniformData.clear();
    state.texturesByUnit.clear();
    if (state.depthStencilState != nullptr) {
        state.depthStencilState->release();
        state.depthStencilState = nullptr;
    }
    if (state.vertexDescriptor != nullptr) {
        state.vertexDescriptor->release();
        state.vertexDescriptor = nullptr;
    }
    states.erase(it);
}

void releaseFramebufferState(Framebuffer *framebuffer) {
    if (framebuffer == nullptr) {
        return;
    }
    auto &states = framebufferStatesStorage();
    auto it = states.find(framebuffer);
    if (it == states.end()) {
        return;
    }
    states.erase(it);
}

void releaseCommandBufferState(CommandBuffer *commandBuffer) {
    if (commandBuffer == nullptr) {
        return;
    }
    auto &states = commandStatesStorage();
    auto it = states.find(commandBuffer);
    if (it == states.end()) {
        return;
    }
    CommandBufferState &state = it->second;
    if (state.encoder != nullptr) {
        state.encoder->endEncoding();
        state.encoder = nullptr;
    }
    state.commandBuffer = nullptr;
    state.passDescriptor = nullptr;
    state.drawable = nullptr;
    state.boundVertexTextures.fill(nullptr);
    state.boundFragmentTextures.fill(nullptr);
    state.boundVertexSamplers.fill(nullptr);
    state.boundFragmentSamplers.fill(nullptr);
    state.textureBindingsInitialized = false;
    state.needsPresent = false;
    state.hasDraw = false;
    state.clearColorPending = false;
    state.clearDepthPending = false;
    if (state.autoreleasePool != nullptr) {
        state.autoreleasePool->release();
        state.autoreleasePool = nullptr;
    }
    states.erase(it);
}

uint32_t registerTextureHandle(const std::shared_ptr<Texture> &texture) {
    if (!texture) {
        return 0;
    }
    uint32_t &nextHandle = nextTextureHandleStorage();
    auto &registry = textureRegistryStorage();
    uint32_t handle = nextHandle++;
    if (handle == 0) {
        handle = nextHandle++;
    }
    registry[handle] = texture;
    return handle;
}

std::shared_ptr<Texture> getTextureFromHandle(uint32_t handle) {
    auto &registry = textureRegistryStorage();
    auto it = registry.find(handle);
    if (it == registry.end()) {
        return nullptr;
    }
    std::shared_ptr<Texture> texture = it->second.lock();
    if (!texture) {
        registry.erase(it);
    }
    return texture;
}

uint32_t stageBindingKey(uint32_t index, bool fragmentStage) {
    return (fragmentStage ? 0x80000000u : 0u) | (index & 0x7fffffffu);
}

MTL::PixelFormat textureFormatToPixelFormat(TextureFormat format) {
    switch (format) {
    case TextureFormat::Rgba8:
        return MTL::PixelFormatRGBA8Unorm;
    case TextureFormat::sRgba8:
        return MTL::PixelFormatRGBA8Unorm_sRGB;
    case TextureFormat::Rgb8:
        return MTL::PixelFormatRGBA8Unorm;
    case TextureFormat::sRgb8:
        return MTL::PixelFormatRGBA8Unorm_sRGB;
    case TextureFormat::Rgba16F:
        return MTL::PixelFormatRGBA16Float;
    case TextureFormat::Rgb16F:
        return MTL::PixelFormatRGBA16Float;
    case TextureFormat::Depth24Stencil8:
        return MTL::PixelFormatDepth32Float_Stencil8;
    case TextureFormat::DepthComponent24:
        return MTL::PixelFormatDepth32Float;
    case TextureFormat::Depth32F:
        return MTL::PixelFormatDepth32Float;
    case TextureFormat::Red8:
        return MTL::PixelFormatR8Unorm;
    case TextureFormat::Red16F:
        return MTL::PixelFormatR16Float;
    default:
        return MTL::PixelFormatRGBA8Unorm;
    }
}

MTL::TextureType textureTypeToMetal(TextureType type) {
    switch (type) {
    case TextureType::Texture2D:
        return MTL::TextureType2D;
    case TextureType::TextureCubeMap:
        return MTL::TextureTypeCube;
    case TextureType::Texture3D:
        return MTL::TextureType3D;
    case TextureType::Texture2DArray:
        return MTL::TextureType2DArray;
    case TextureType::Texture2DMultisample:
        return MTL::TextureType2DMultisample;
    default:
        return MTL::TextureType2D;
    }
}

bool isDepthFormat(TextureFormat format) {
    return format == TextureFormat::Depth24Stencil8 ||
           format == TextureFormat::DepthComponent24 ||
           format == TextureFormat::Depth32F;
}

MTL::TextureUsage textureUsageFor(TextureType type, TextureFormat format) {
    if (type == TextureType::Texture3D) {
        return enumOr(MTL::TextureUsageShaderRead, MTL::TextureUsageShaderWrite);
    }
    if (isDepthFormat(format)) {
        return enumOr(MTL::TextureUsageShaderRead, MTL::TextureUsageRenderTarget);
    }
    return enumOr(MTL::TextureUsageShaderRead, MTL::TextureUsageRenderTarget);
}

MTL::SamplerAddressMode wrapModeToAddressMode(TextureWrapMode mode) {
    switch (mode) {
    case TextureWrapMode::Repeat:
        return MTL::SamplerAddressModeRepeat;
    case TextureWrapMode::MirroredRepeat:
        return MTL::SamplerAddressModeMirrorRepeat;
    case TextureWrapMode::ClampToEdge:
        return MTL::SamplerAddressModeClampToEdge;
    case TextureWrapMode::ClampToBorder:
        return MTL::SamplerAddressModeClampToBorderColor;
    default:
        return MTL::SamplerAddressModeRepeat;
    }
}

void rebuildTextureSampler(Texture *texture, MTL::Device *device) {
    if (texture == nullptr || device == nullptr) {
        return;
    }

    TextureState &state = textureState(texture);
    MTL::SamplerDescriptor *descriptor =
        MTL::SamplerDescriptor::alloc()->init();

    descriptor->setSAddressMode(wrapModeToAddressMode(state.wrapS));
    descriptor->setTAddressMode(wrapModeToAddressMode(state.wrapT));
    descriptor->setRAddressMode(wrapModeToAddressMode(state.wrapR));

    auto toMinMag = [](TextureFilterMode mode) {
        switch (mode) {
        case TextureFilterMode::Nearest:
        case TextureFilterMode::NearestMipmapNearest:
            return MTL::SamplerMinMagFilterNearest;
        case TextureFilterMode::Linear:
        case TextureFilterMode::LinearMipmapLinear:
            return MTL::SamplerMinMagFilterLinear;
        default:
            return MTL::SamplerMinMagFilterLinear;
        }
    };

    auto toMip = [](TextureFilterMode mode) {
        switch (mode) {
        case TextureFilterMode::NearestMipmapNearest:
            return MTL::SamplerMipFilterNearest;
        case TextureFilterMode::LinearMipmapLinear:
            return MTL::SamplerMipFilterLinear;
        default:
            return MTL::SamplerMipFilterNotMipmapped;
        }
    };

    descriptor->setMinFilter(toMinMag(state.minFilter));
    descriptor->setMagFilter(toMinMag(state.magFilter));
    descriptor->setMipFilter(toMip(state.minFilter));

    if (state.wrapS == TextureWrapMode::ClampToBorder ||
        state.wrapT == TextureWrapMode::ClampToBorder ||
        state.wrapR == TextureWrapMode::ClampToBorder) {
        float avg = (state.borderColor.r + state.borderColor.g +
                     state.borderColor.b) /
                    3.0f;
        if (avg > 0.75f) {
            descriptor->setBorderColor(MTL::SamplerBorderColorOpaqueWhite);
        } else {
            descriptor->setBorderColor(MTL::SamplerBorderColorOpaqueBlack);
        }
    }

    if (state.sampler != nullptr) {
        state.sampler->release();
    }
    state.sampler = device->newSamplerState(descriptor);
    descriptor->release();
}

bool parseProgramLayouts(const std::string &vertexSource,
                         const std::string &fragmentSource,
                         ProgramState &state) {
    state.layouts.clear();
    state.bindings.clear();
    state.bindingSize.clear();
    state.uniformResolutionCache.clear();

    std::unordered_map<std::string, RawStruct> rawStructs =
        parseRawStructs(vertexSource);
    std::unordered_map<std::string, RawStruct> fragmentStructs =
        parseRawStructs(fragmentSource);
    for (const auto &pair : fragmentStructs) {
        if (rawStructs.find(pair.first) == rawStructs.end()) {
            rawStructs[pair.first] = pair.second;
        }
    }

    std::vector<BufferBinding> vertexBindings =
        parseStageBufferBindings(vertexSource, true);
    std::vector<BufferBinding> fragmentBindings =
        parseStageBufferBindings(fragmentSource, false);
    vertexBindings.insert(vertexBindings.end(), fragmentBindings.begin(),
                          fragmentBindings.end());

    for (const BufferBinding &binding : vertexBindings) {
        bool merged = false;
        for (BufferBinding &existing : state.bindings) {
            if (existing.index == binding.index &&
                existing.structName == binding.structName) {
                existing.vertexStage = existing.vertexStage || binding.vertexStage;
                existing.fragmentStage =
                    existing.fragmentStage || binding.fragmentStage;
                bool existingAnonymous = existing.instanceName.empty() ||
                                         existing.instanceName[0] == '_';
                bool incomingNamed = !binding.instanceName.empty() &&
                                     binding.instanceName[0] != '_';
                if (existingAnonymous && incomingNamed) {
                    existing.instanceName = binding.instanceName;
                }
                merged = true;
                break;
            }
        }
        if (!merged) {
            state.bindings.push_back(binding);
        }
    }

    std::unordered_map<std::string, StructLayout> cache;
    std::unordered_set<std::string> visiting;
    for (const BufferBinding &binding : state.bindings) {
        StructLayout layout =
            computeLayout(binding.structName, rawStructs, cache, visiting);
        if (layout.size == 0) {
            continue;
        }
        state.layouts[binding.structName] = layout;
        if (binding.vertexStage) {
            uint32_t vertexKey = stageBindingKey(binding.index, false);
            state.bindingSize[vertexKey] =
                std::max(state.bindingSize[vertexKey], layout.size);
        }
        if (binding.fragmentStage) {
            uint32_t fragmentKey = stageBindingKey(binding.index, true);
            state.bindingSize[fragmentKey] =
                std::max(state.bindingSize[fragmentKey], layout.size);
        }
    }

    return state.vertexFunction != nullptr && state.fragmentFunction != nullptr;
}

std::vector<UniformLocation> resolveUniformLocations(ProgramState &programState,
                                                     const std::string &name) {
    auto cacheIt = programState.uniformResolutionCache.find(name);
    if (cacheIt != programState.uniformResolutionCache.end()) {
        return cacheIt->second;
    }

    std::vector<UniformLocation> resolved;
    const std::vector<std::string> tokens = parseUniformPath(name);
    if (tokens.empty()) {
        programState.uniformResolutionCache[name] = resolved;
        return resolved;
    }

    for (const BufferBinding &binding : programState.bindings) {
        auto layoutIt = programState.layouts.find(binding.structName);
        if (layoutIt == programState.layouts.end()) {
            continue;
        }

        const StructLayout *layout = &layoutIt->second;
        size_t offset = 0;
        size_t finalSize = 0;
        bool ok = true;
        size_t tokenStart = 0;
        if (!tokens.empty() && matchesBindingAlias(binding, tokens[0])) {
            tokenStart = 1;
        }

        if (tokenStart == tokens.size()) {
            finalSize = layout->size;
        }

        for (size_t tokenIndex = tokenStart; tokenIndex < tokens.size();
             ++tokenIndex) {
            std::string fieldName;
            bool hasIndex = false;
            size_t index = 0;
            if (!parseUniformToken(tokens[tokenIndex], fieldName, hasIndex,
                                   index)) {
                ok = false;
                break;
            }

            const StructField *field = findField(*layout, fieldName);
            if (field == nullptr) {
                ok = false;
                break;
            }

            size_t effectiveIndex = hasIndex ? index : 0;
            offset += field->offset + effectiveIndex * field->stride;

            bool isLast = (tokenIndex + 1 == tokens.size());
            if (isLast) {
                if (field->type.isStruct) {
                    finalSize = hasIndex ? field->type.size
                                         : field->stride * field->arrayCount;
                } else {
                    finalSize = hasIndex ? field->type.size
                                         : ((field->arrayCount > 1)
                                                ? field->stride * field->arrayCount
                                                : field->type.size);
                }
            } else {
                if (!field->type.isStruct) {
                    ok = false;
                    break;
                }
                auto nestedIt =
                    programState.layouts.find(field->type.structName);
                if (nestedIt == programState.layouts.end()) {
                    ok = false;
                    break;
                }
                layout = &nestedIt->second;
            }
        }

        if (!ok || finalSize == 0) {
            continue;
        }

        UniformLocation location;
        location.bufferIndex = binding.index;
        location.offset = offset;
        location.size = finalSize;
        location.vertexStage = binding.vertexStage;
        location.fragmentStage = binding.fragmentStage;

        resolved.push_back(location);
    }

    programState.uniformResolutionCache[name] = resolved;
    return resolved;
}

std::string makePipelineKey(const std::array<MTL::PixelFormat, 8> &colors,
                            uint32_t colorCount, MTL::PixelFormat depthFormat,
                            MTL::PixelFormat stencilFormat,
                            uint32_t sampleCount) {
    std::string key = std::to_string(colorCount) + "|" +
                      std::to_string(static_cast<uint32_t>(depthFormat)) + "|" +
                      std::to_string(static_cast<uint32_t>(stencilFormat)) + "|" +
                      std::to_string(sampleCount);
    for (uint32_t i = 0; i < colorCount && i < colors.size(); ++i) {
        key += "|" + std::to_string(static_cast<uint32_t>(colors[i]));
    }
    return key;
}

uint32_t fragmentColorOutputCount(const std::string &fragmentSource) {
    static const std::regex colorRegex(R"(\[\[\s*color\((\d+)\)\s*\]\])");
    auto begin =
        std::sregex_iterator(fragmentSource.begin(), fragmentSource.end(),
                             colorRegex);
    auto end = std::sregex_iterator();
    int maxIndex = -1;
    for (auto it = begin; it != end; ++it) {
        maxIndex = std::max(maxIndex, std::stoi((*it)[1].str()));
    }
    return maxIndex < 0 ? 0 : static_cast<uint32_t>(maxIndex + 1);
}

} // namespace opal::metal

#endif
