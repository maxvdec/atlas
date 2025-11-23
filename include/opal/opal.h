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

#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <sys/types.h>
#include <vector>

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

class Device {
  public:
    static std::shared_ptr<Device> acquire(std::shared_ptr<Context> context);
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

    inline bool operator==(const VertexAttribute &other) const {
        return name == other.name && type == other.type &&
               offset == other.offset && location == other.location &&
               normalized == other.normalized && size == other.size;
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

    void enableBlending(bool enabled);
    void setBlendFunc(BlendFunc srcFactor, BlendFunc dstFactor);

    template <typename T>
    void setUniform(const std::string &name, const T &value);

    void build();

    void bind();

    bool operator==(std::shared_ptr<Pipeline> pipeline) const;

    std::shared_ptr<ShaderProgram> shaderProgram;

  private:
    PrimitiveStyle primitiveStyle;
    RasterizerMode rasterizerMode;
    CullMode cullMode;
    FrontFace frontFace;
    bool blendingEnabled = false;
    BlendFunc blendSrcFactor = BlendFunc::One;
    BlendFunc blendDstFactor = BlendFunc::Zero;
    bool depthTestEnabled = false;
    CompareOp depthCompareOp = CompareOp::Less;

    std::vector<VertexAttribute> vertexAttributes;
    VertexBinding vertexBinding;

    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 800;
    int viewportHeight = 600;

    uint getGLBlendFactor(BlendFunc factor);
    uint getGLCompareOp(CompareOp op);
    uint getGLPrimitiveStyle(PrimitiveStyle style);
    uint getGLRasterizerMode(RasterizerMode mode);
    uint getGLCullMode(CullMode mode);
    uint getGLFrontFace(FrontFace face);
    uint getGLVertexAttributeType(VertexAttributeType type);
};

} // namespace opal

#endif // OPAL_H
