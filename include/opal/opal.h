/*
 opal.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: The main opal core rendering implementation
 Copyright (c) 2025 maxvdec
*/

#ifndef OPAL_H
#define OPAL_H

#include <cstddef>
#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <glm/glm.hpp>

namespace opal {

enum class OpenGLProfile { Core, Compatibility };

struct ContextConfiguration {
    bool useOpenGL = true;
    int majorVersion = 4;
    int minorVersion = 1;
    OpenGLProfile profile = OpenGLProfile::Core;
};

class Context {
  public:
    static std::shared_ptr<Context> create(ContextConfiguration config = {});

    void setFlag(int flag, bool enabled);
    void setFlag(int flag, int value);

    GLFWwindow *makeWindow(int width, int height, const char *title,
                           GLFWmonitor *monitor = nullptr,
                           GLFWwindow *share = nullptr);
    GLFWwindow *getWindow() const;

    void makeCurrent();

  private:
    GLFWwindow *window = nullptr;
};

class CommandBuffer;
class Framebuffer;

class Device {
  public:
    static std::shared_ptr<Device> acquire(std::shared_ptr<Context> context);
    static std::shared_ptr<CommandBuffer> acquireCommandBuffer();

    void submitCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer);
    std::shared_ptr<Framebuffer> getDefaultFramebuffer();
};

enum class TextureType {
    Texture2D,
    TextureCubeMap,
    Texture3D,
    Texture2DArray,
    Texture2DMultisample
};
enum class TextureFormat {
    Rgba8,
    sRgba8,
    Rgb8,
    sRgb8,
    Rgba16F,
    Rgb16F,
    Depth24Stencil8,
    DepthComponent24,
    Depth32F,
    Red8,
    Red16F
};

enum class TextureWrapMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};
enum class TextureFilterMode {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapLinear
};

enum class TextureAxis { S, T, R };

enum class TextureDataFormat { Rgba, Rgb, Red, Bgr, Bgra, DepthComponent };

class Texture {
  public:
    static std::shared_ptr<Texture>
    create(TextureType type, TextureFormat format, int width, int height,
           TextureDataFormat dataFormat = TextureDataFormat::Rgba,
           const void *data = nullptr, uint mipLevels = 1);

    /**
     * @brief Creates a multisampled texture for use with multisampled
     * framebuffers.
     * @param format The texture format.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param samples Number of samples (typically 2, 4, 8, or 16).
     * @return A shared pointer to the created multisampled texture.
     */
    static std::shared_ptr<Texture> createMultisampled(TextureFormat format,
                                                       int width, int height,
                                                       int samples = 4);

    /**
     * @brief Creates a depth cubemap texture for omnidirectional shadow
     * mapping.
     * @param format The depth format (should be DepthComponent24 or Depth32F).
     * @param resolution The resolution of each face (width = height).
     * @return A shared pointer to the created depth cubemap texture.
     */
    static std::shared_ptr<Texture> createDepthCubemap(TextureFormat format,
                                                       int resolution);

    void updateFace(int faceIndex, const void *data, int width, int height,
                    TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void updateData3D(const void *data, int width, int height, int depth,
                      TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void updateData(const void *data, int width, int height,
                    TextureDataFormat dataFormat = TextureDataFormat::Rgba);
    void changeFormat(TextureFormat newFormat);
    void changeBorderColor(const glm::vec4 &borderColor);

    void generateMipmaps(uint levels);
    void automaticallyGenerateMipmaps();
    void setWrapMode(TextureAxis axis, TextureWrapMode mode);
    void setFilterMode(TextureFilterMode minFilter,
                       TextureFilterMode magFilter);

    // Batch parameter setter - sets all common parameters in one bind
    void setParameters(TextureWrapMode wrapS, TextureWrapMode wrapT,
                       TextureFilterMode minFilter,
                       TextureFilterMode magFilter);
    void setParameters3D(TextureWrapMode wrapS, TextureWrapMode wrapT,
                         TextureWrapMode wrapR, TextureFilterMode minFilter,
                         TextureFilterMode magFilter);

    uint textureID;
    TextureType type;
    TextureFormat format;
    int width;
    int height;
    int samples = 1; // For multisampled textures

  private:
    friend class Pipeline;
    friend class CommandBuffer;
    friend class Framebuffer;

    uint glType = 0;
    uint glFormat = 0;
};

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    TessellationControl,
    TessellationEvaluation
};

class Shader {
  public:
    static std::shared_ptr<Shader> createFromSource(const char *source,
                                                    ShaderType type);

    void compile();

    bool getShaderStatus() const;
    void getShaderLog(char *logBuffer, size_t bufferSize) const;

    uint shaderID;
    ShaderType type;

  private:
#ifdef OPENGL
    static uint getGLShaderType(ShaderType type);
#endif
};

class ShaderProgram {
  public:
    static std::shared_ptr<ShaderProgram> create();
    void attachShader(std::shared_ptr<Shader> shader);

    void link();
    void use();

    bool getProgramStatus() const;
    void getProgramLog(char *logBuffer, size_t bufferSize) const;

    uint programID;
    std::vector<std::shared_ptr<Shader>> attachedShaders;
};

enum class VertexAttributeType {
    Float,
    Double,
    Int,
    UnsignedInt,
    Short,
    UnsignedShort,
    Byte,
    UnsignedByte
};

enum class PrimitiveStyle {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

enum class RasterizerMode { Fill, Line, Point };

enum class VertexBindingInputRate { Vertex, Instance };

enum class CullMode { None, Front, Back, FrontAndBack };

enum class FrontFace { Clockwise, CounterClockwise };

enum class CompareOp {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class BlendFunc {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha
};

struct VertexAttribute {
    std::string name;
    VertexAttributeType type;
    uint offset;
    uint location;
    bool normalized = false;
    uint size;
    uint stride;
    VertexBindingInputRate inputRate = VertexBindingInputRate::Vertex;
    uint divisor = 0;

    inline bool operator==(const VertexAttribute &other) const {
        return name == other.name && type == other.type &&
               offset == other.offset && location == other.location &&
               normalized == other.normalized && size == other.size &&
               stride == other.stride && inputRate == other.inputRate &&
               divisor == other.divisor;
    }
};

struct VertexBinding {
    uint stride;
    VertexBindingInputRate inputRate;
};

class Pipeline {
  public:
    static std::shared_ptr<Pipeline> create();

    void setShaderProgram(std::shared_ptr<ShaderProgram> program);

    void setVertexAttributes(const std::vector<VertexAttribute> &attributes,
                             const VertexBinding &binding);

    void setPrimitiveStyle(PrimitiveStyle style);

    void setViewport(int x, int y, int width, int height);

    void setRasterizerMode(RasterizerMode mode);

    void setCullMode(CullMode mode);

    void setFrontFace(FrontFace face);

    void enableDepthTest(bool enabled);
    void setDepthCompareOp(CompareOp op);
    void enableDepthWrite(bool enabled);

    void enableBlending(bool enabled);
    void setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor);

    void enableMultisampling(bool enabled);
    void enablePolygonOffset(bool enabled);
    void setPolygonOffset(float factor, float units);
    void enableClipDistance(int index, bool enabled);

    void build();

    void bind();

    bool operator==(std::shared_ptr<Pipeline> pipeline) const;

    std::shared_ptr<ShaderProgram> shaderProgram;

    void setUniform3f(const std::string &name, float v0, float v1, float v2);
    void setUniform1i(const std::string &name, int v0);
    void setUniformMat4f(const std::string &name, const glm::mat4 &matrix);
    void setUniform1f(const std::string &name, float v0);
    void setUniformBool(const std::string &name, bool value);
    void setUniform2f(const std::string &name, float v0, float v1);
    void setUniform4f(const std::string &name, float v0, float v1, float v2,
                      float v3);
    void bindTexture(const std::string &name, std::shared_ptr<Texture> texture,
                     int unit);
    void bindTexture2D(const std::string &name, uint textureId, int unit);
    void bindTextureCubemap(const std::string &name, uint textureId, int unit);

  private:
    PrimitiveStyle primitiveStyle;
    RasterizerMode rasterizerMode;
    CullMode cullMode;
    FrontFace frontFace;
    bool blendingEnabled = false;
    BlendFunc blendSrcFactor = BlendFunc::One;
    BlendFunc blendDstFactor = BlendFunc::Zero;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    CompareOp depthCompareOp = CompareOp::Less;
    bool multisamplingEnabled = false;
    bool polygonOffsetEnabled = false;
    float polygonOffsetFactor = 0.0f;
    float polygonOffsetUnits = 0.0f;
    std::vector<int> enabledClipDistances;

    std::vector<VertexAttribute> vertexAttributes;
    VertexBinding vertexBinding;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 800;
    int viewportHeight = 600;

    uint getGLBlendFactor(BlendFunc factor) const;
    uint getGLCompareOp(CompareOp op) const;
    uint getGLPrimitiveStyle(PrimitiveStyle style) const;
    uint getGLRasterizerMode(RasterizerMode mode) const;
    uint getGLCullMode(CullMode mode) const;
    uint getGLFrontFace(FrontFace face) const;
    uint getGLVertexAttributeType(VertexAttributeType type) const;
};

enum class BufferUsage {
    VertexBuffer,
    IndexArray,
    GeneralPurpose,
    UniformBuffer,
    ShaderRead,
    ShaderReadWrite
};

enum class MemoryUsageType { GPUOnly, CPUToGPU, GPUToCPU };

class Buffer {
  public:
    static std::shared_ptr<Buffer>
    create(BufferUsage usage, size_t size, const void *data = nullptr,
           MemoryUsageType memoryUsage = MemoryUsageType::GPUOnly);

    void updateData(size_t offset, size_t size, const void *data);

    void bind() const;
    void unbind() const;

    uint bufferID;

    BufferUsage usage;
    MemoryUsageType memoryUsage;
};

struct VertexAttributeBinding {
    VertexAttribute attribute;
    std::shared_ptr<Buffer> sourceBuffer = nullptr;
};

struct DrawingState {
    std::shared_ptr<Buffer> vertexBuffer = nullptr;
    std::shared_ptr<Buffer> indexBuffer = nullptr;

    static std::shared_ptr<DrawingState>
    create(std::shared_ptr<Buffer> vertexBuffer,
           std::shared_ptr<Buffer> indexBuffer = nullptr);

    void setBuffers(std::shared_ptr<Buffer> vertexBuffer,
                    std::shared_ptr<Buffer> indexBuffer = nullptr);

    void bind() const;
    void unbind() const;
    void configureAttributes(
        const std::vector<VertexAttributeBinding> &bindings) const;

    uint index;
};

class Attachment {
  public:
    enum class Type { Color, Depth, Stencil, DepthStencil };

    Type type;

    std::shared_ptr<Texture> texture;
};

class DepthStencilBuffer {
  public:
    static std::shared_ptr<DepthStencilBuffer>
    create(int width, int height,
           TextureFormat format = TextureFormat::Depth24Stencil8);

    std::shared_ptr<Texture> texture;

    uint renderbufferID;
};

class Framebuffer {
  public:
    static std::shared_ptr<Framebuffer> create(int width, int height);

    void addAttachment(const Attachment &attachment);
    void attachDepthStencilBuffer(
        std::shared_ptr<DepthStencilBuffer> depthStencilBuffer);

    /**
     * @brief Attaches a cubemap texture to this framebuffer for omnidirectional
     * rendering (e.g., point light shadows).
     * @param texture The cubemap texture to attach.
     * @param attachmentType The attachment type (typically Depth for shadows).
     */
    void attachCubemap(std::shared_ptr<Texture> texture,
                       Attachment::Type attachmentType);

    /**
     * @brief Disables color buffer read/write for depth-only rendering.
     * Used for shadow map generation.
     */
    void disableColorBuffer();

    /**
     * @brief Sets the viewport to match this framebuffer's dimensions.
     */
    void setViewport();

    /**
     * @brief Sets the viewport to a custom size.
     */
    void setViewport(int x, int y, int viewWidth, int viewHeight);

    void bind();
    void unbind();

    void bindForRead();
    void bindForDraw();

    /**
     * @brief Sets which color attachments to draw to.
     * @param attachmentCount Number of color attachments to enable (0-N).
     */
    void setDrawBuffers(int attachmentCount);

    bool getStatus() const;

    uint framebufferID;

    int width;
    int height;
    std::vector<Attachment> attachments;

  private:
    bool colorBufferDisabled = false;
};

class RenderPass {
  public:
    static std::shared_ptr<RenderPass> create();

    void setFramebuffer(std::shared_ptr<Framebuffer> framebuffer);

    std::shared_ptr<Framebuffer> framebuffer;
};

class ResolveAction {
  public:
    static std::shared_ptr<ResolveAction>
    create(std::shared_ptr<Framebuffer> source,
           std::shared_ptr<Framebuffer> destination);

    /**
     * @brief Creates a resolve action for a specific color attachment.
     * @param source The source (multisampled) framebuffer.
     * @param destination The destination (regular) framebuffer.
     * @param colorAttachmentIndex The index of the color attachment to resolve.
     * @return A shared pointer to the resolve action.
     */
    static std::shared_ptr<ResolveAction>
    createForColorAttachment(std::shared_ptr<Framebuffer> source,
                             std::shared_ptr<Framebuffer> destination,
                             int colorAttachmentIndex);

    /**
     * @brief Creates a resolve action for the depth buffer only.
     */
    static std::shared_ptr<ResolveAction>
    createForDepth(std::shared_ptr<Framebuffer> source,
                   std::shared_ptr<Framebuffer> destination);

    std::shared_ptr<Framebuffer> source;
    std::shared_ptr<Framebuffer> destination;
    int colorAttachmentIndex = -1; // -1 means all color attachments
    bool resolveDepth = true;
    bool resolveColor = true;
};

class CommandBuffer {
  public:
    void start();
    void beginPass(std::shared_ptr<RenderPass> renderPass);
    void beginSampled(std::shared_ptr<Framebuffer> readFramebuffer,
                      std::shared_ptr<Framebuffer> writeFramebuffer);
    void cancelPass();
    void endPass();
    void commit();

    // The different commands
    void bindPipeline(std::shared_ptr<Pipeline> pipeline);
    void unbindPipeline();
    void bindDrawingState(std::shared_ptr<DrawingState> drawingState);
    void unbindDrawingState();
    void draw(uint vertexCount, uint instanceCount = 1, uint firstVertex = 0,
              [[maybe_unused]] uint firstInstance = 0);
    void drawIndexed(uint indexCount, uint instanceCount = 1,
                     uint firstIndex = 0, int vertexOffset = 0,
                     uint firstInstance = 0);
    void performResolve(std::shared_ptr<ResolveAction> resolveAction);

    void clearColor(float r, float g, float b, float a);
    void clearDepth(float depth);
    void clear(float r, float g, float b, float a, float depth);

  private:
    std::shared_ptr<Pipeline> boundPipeline = nullptr;
    std::shared_ptr<DrawingState> boundDrawingState = nullptr;
};

} // namespace opal

#endif // OPAL_H
