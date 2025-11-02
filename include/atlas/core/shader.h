/*
 shader.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Shader definition and structure
 Copyright (c) 2025 maxvdec
*/

#ifndef SHADER_H
#define SHADER_H

#include "atlas/units.h"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#define DEFAULT_FRAG_SHADER Main
#define DEFAULT_VERT_SHADER Main

/**
 * @brief  Enumeration of default vertex shaders provided by the Atlas engine.
 *
 */
enum class AtlasVertexShader {
    /**
     * @brief Debug vertex shader that outputs a solid magenta color.
     *
     */
    Debug,
    /**
     * @brief Vertex shader for rendering solid colors.
     *
     */
    Color,
    /**
     * @brief Main vertex shader that supports lighting, textures, and normals.
     *
     */
    Main,
    /**
     * @brief Vertex shader for rendering textured objects.
     *
     */
    Texture,
    /**
     * @brief Vertex shader for rendering fullscreen quads.
     *
     */
    Fullscreen,
    /**
     * @brief Vertex shader for rendering skyboxes.
     *
     */
    Skybox,
    /**
     * @brief Vertex shader for rendering depth maps (used in shadow mapping).
     *
     */
    Depth,
    /**
     * @brief Vertex shader for rendering particles.
     *
     */
    Particle,
    /**
     * @brief Vertex shader for rendering text.
     *
     */
    Text,
    /**
     * @brief Vertex shader for rendering point light shadow maps.
     *
     */
    PointLightShadow,
    /**
     * @brief Vertex shader for rendering directional light shadows.
     *
     */
    Light,
    /**
     * @brief Vertex shader for rendering deferred shading.
     *
     */
    Deferred,
    /**
     * @brief Vertex shader for rendering terrain.
     *
     */
    Terrain,
    /**
     * @brief Vertex shader tailored for volumetric light scattering passes.
     */
    Volumetric,
    Fluid,
};

/**
 * @brief Enumeration of the capabilities a shader can have.
 *
 */
enum class ShaderCapability {
    /**
     * @brief Capability for handling lighting calculations.
     *
     */
    Lighting,
    /**
     * @brief Capability for handling texture mapping.
     *
     */
    Textures,
    /**
     * @brief Capability for handling shadow mapping.
     *
     */
    Shadows,
    /**
     * @brief Capability for handling environment mapping (e.g., reflections).
     *
     */
    /**
     * @brief Enables sampling of cube maps or HDR textures for reflections.
     */
    EnvironmentMapping,
    /**
     * @brief Supports image-based lighting sampling and BRDF integration.
     */
    IBL,
    /**
     * @brief Capability for handling skeletal animations.
     *
     */
    Deferred,
    /**
     * @brief Capability for handling deferred rendering.
     *
     */
    /**
     * @brief Indicates the shader participates in the lighting pass of deferred
     * rendering.
     */
    LightDeferred,
    /**
     * @brief Capability for handling material properties.
     *
     */
    Material,
    /**
     * @brief Capability for handling instancing (rendering multiple objects
     * with a single draw call).
     *
     */
    Instances,
    /**
     * @brief Provides access to environment parameters (fog, rim light, etc.).
     */
    Environment,
    Fluid,
};

/**
 * @brief Structure representing a vertex shader, including its source code,
 *
 */
struct VertexShader {
    /**
     * @brief The source code of the vertex shader.
     *
     */
    const char *source;

    /**
     * @brief Static cache of compiled vertex shaders to avoid recompilation.
     *
     */
    static std::map<AtlasVertexShader, VertexShader> vertexShaderCache;

    /**
     * @brief If this shader was created from a default shader, stores which
     * type it was.
     *
     */
    std::optional<AtlasVertexShader> fromDefaultShaderType = std::nullopt;

    /**
     * @brief A function that creates a VertexShader from a default shader.
     *
     * @param shader The type of default shader to create.
     * @return The created VertexShader instance.
     */
    static VertexShader fromDefaultShader(AtlasVertexShader shader);
    /**
     * @brief A function that creates a VertexShader from custom source code.
     *
     * @param source The source code that the shader will contain.
     * @return The created VertexShader instance.
     */
    static VertexShader fromSource(const char *source);
    /**
     * @brief Compiles the vertex shader.
     *
     */
    void compile();

    /**
     * @brief The desired vertex attributes for the shader.
     *
     */
    std::vector<uint32_t> desiredAttributes;
    /**
     * @brief The capabilities of the shader.
     *
     */
    std::vector<ShaderCapability> capabilities;

    /**
     * @brief The OpenGL ID of the compiled shader.
     *
     */
    Id shaderId;
};

/**
 * @brief  Enumeration of default fragment shaders provided by the Atlas engine.
 *
 */
enum class AtlasFragmentShader {
    /**
     * @brief Debug fragment shader that outputs a solid magenta color.
     *
     */
    Debug,
    /**
     * @brief Fragment shader for rendering solid colors.
     *
     */
    Color,
    /**
     * @brief Main fragment shader that supports lighting, textures, and
     * normals.
     *
     */
    Main,
    /**
     * @brief Fragment shader for rendering textured objects.
     *
     */
    Texture,
    /**
     * @brief Fragment shader for rendering fullscreen quads.
     *
     */
    Fullscreen,
    /**
     * @brief Fragment shader for rendering skyboxes.
     *
     */
    Skybox,
    /**
     * @brief Fragment shader for rendering empty objects.
     *
     */
    Empty,
    /**
     * @brief Fragment shader for rendering particles.
     *
     */
    Particle,
    /**
     * @brief Fragment shader for rendering text.
     *
     */
    Text,
    /**
     * @brief Fragment shader for rendering depth maps (used in shadow mapping).
     *
     */
    PointLightShadow,
    /**
     * @brief Fragment shader for applying a Gaussian blur effect.
     *
     */
    GaussianBlur,
    /**
     * @brief Main fragment shader for rendering lights in deferred rendering.
     *
     */
    Light,
    /**
     * @brief Main fragment shader for deferred rendering.
     *
     */
    Deferred,
    /**
     * @brief Fragment shader for rendering screen-space ambient occlusion
     * (SSAO).
     *
     */
    SSAO,
    /**
     * @brief Fragment shader for blurring the SSAO texture.
     *
     */
    SSAOBlur,
    /**
     * @brief Fragment shader responsible for tessellated terrain shading.
     */
    Terrain,
    /**
     * @brief Fragment shader used for volumetric lighting integration.
     */
    Volumetric,
    /**
     * @brief Fragment shader that downsamples textures within bloom chains.
     */
    Downsample,
    /**
     * @brief Fragment shader that upsamples and blends bloom textures.
     */
    Upsample,
    Fluid,
};

/**
 * @brief Structure representing a fragment shader, including its source code
 * and its OpenGL ID.
 *
 */
struct FragmentShader {
    /**
     * @brief The source code of the fragment shader.
     *
     */
    const char *source;

    /**
     * @brief Static cache of compiled fragment shaders to avoid recompilation.
     *
     */
    static std::map<AtlasFragmentShader, FragmentShader> fragmentShaderCache;

    /**
     * @brief If this shader was created from a default shader, stores which
     * type it was.
     *
     */
    std::optional<AtlasFragmentShader> fromDefaultShaderType = std::nullopt;

    /**
     * @brief Creates a FragmentShader from a default shader.
     *
     * @param shader The type of default shader to create.
     * @return The created FragmentShader instance.
     */
    static FragmentShader fromDefaultShader(AtlasFragmentShader shader);
    /**
     * @brief Creates a FragmentShader from custom source code.
     *
     * @param source The source code that the shader will contain.
     * @return The created FragmentShader instance.
     */
    static FragmentShader fromSource(const char *source);
    /**
     * @brief Function to compile the fragment shader.
     *
     */
    void compile();

    /**
     * @brief The desired vertex attributes for the shader.
     *
     */
    Id shaderId;
};

/**
 * @brief Enumeration of default geometry shaders provided by the Atlas engine.
 *
 */
enum class AtlasGeometryShader {
    /**
     * @brief Geometry shader for rendering point light shadow maps.
     *
     */
    PointLightShadow,
};

/**
 * @brief Structure representing a geometry shader, including its source code
 * and its OpenGL ID.
 *
 */
struct GeometryShader {
    /**
     * @brief The source code of the geometry shader.
     *
     */
    const char *source;

    /**
     * @brief Creates a GeometryShader from a default shader.
     *
     * @param shader The type of default shader to create.
     * @return The created GeometryShader instance.
     */
    static GeometryShader fromDefaultShader(AtlasGeometryShader shader);
    /**
     * @brief Creates a GeometryShader from custom source code.
     *
     * @param source The source code that the shader will contain.
     * @return The created GeometryShader instance.
     */
    static GeometryShader fromSource(const char *source);
    /**
     * @brief Function to compile the geometry shader.
     *
     */
    void compile();

    /**
     * @brief The desired vertex attributes for the shader.
     *
     */
    Id shaderId;
};

enum class AtlasTessellationShader {
    /**
     * @brief Control stage shader used for adaptive terrain tessellation.
     */
    TerrainControl,
    /**
     * @brief Evaluation stage shader producing displaced terrain vertices.
     */
    TerrainEvaluation,
    /**
     * @brief Primitive generation shader defining tessellated patch layout.
     */
    TerrainPrimitive
};

enum class TessellationShaderType { Control, Evaluation, Primitive };

class TessellationShader {
  public:
    /**
     * @brief The source code of the tessellation shader.
     *
     */
    const char *source;

    /**
     * @brief The type of the tessellation shader.
     *
     */
    TessellationShaderType type;

    /**
     * @brief Creates a TessellationShader from a default shader.
     *
     * @param shader The type of default shader to create.
     * @return The created TessellationShader instance.
     */
    static TessellationShader fromDefaultShader(AtlasTessellationShader shader);

    /**
     * @brief Creates a TessellationShader from custom source code.
     *
     * @param source The source code that the shader will contain.
     * @return The created TessellationShader instance.
     */
    static TessellationShader fromSource(const char *source,
                                         TessellationShaderType type);

    /**
     * @brief Function to compile the tessellation shader.
     *
     */
    void compile();

    /**
     * @brief The OpenGL ID of the compiled shader.
     *
     */
    Id shaderId;
};

/**
 * @brief Structure representing a layout descriptor for vertex attributes.
 *
 */
struct LayoutDescriptor {
    /**
     * @brief The layout position of the attribute in the shader.
     *
     */
    int layoutPos;
    /**
     * @brief The size of the attribute data type (e.g., 3 for vec3).
     *
     */
    int size;
    /**
     * @brief The OpenGL data type of the attribute (e.g., GL_FLOAT).
     *
     */
    unsigned int type;
    /**
     * @brief Whether the attribute data should be normalized.
     *
     */
    bool normalized;
    /**
     * @brief The stride (in bytes) between consecutive attributes.
     *
     */
    int stride;
    /**
     * @brief The byte offset of the attribute within the vertex data.
     *
     */
    std::size_t offset;
};

/**
 * @brief Structure representing a complete shader program, consisting of a
 * vertex shader and a fragment shader.
 *
 */
struct ShaderProgram {
    /**
     * @brief The vertex shader component of the shader program.
     *
     */
    VertexShader vertexShader;
    /**
     * @brief The fragment shader component of the shader program.
     *
     */
    FragmentShader fragmentShader;

    /**
     * @brief The geometry shader component of the shader program (optional).
     *
     */
    GeometryShader geometryShader;

    /**
     * @brief The tessellation shader component of the shader program
     * (optional).
     *
     */
    std::vector<TessellationShader> tessellationShaders;

    /**
     * @brief Static cache of compiled shader programs to avoid recompilation.
     *
     */
    static std::map<std::pair<AtlasVertexShader, AtlasFragmentShader>,
                    ShaderProgram>
        shaderCache;

    /**
     * @brief Compiles the shader program by linking the vertex and fragment
     * shaders and destroyes the original shaders after linking.
     *
     */
    void compile();

    /**
     * @brief Creates a default shader program with predefined vertex and
     * fragment (main) shaders.
     *
     * @return The created default ShaderProgram instance.
     */
    static ShaderProgram defaultProgram();
    /**
     * @brief Creates a ShaderProgram from specified default vertex and fragment
     * shaders.
     *
     * @param vShader The type of default vertex shader to use.
     * @param fShader The type of default fragment shader to use.
     * @return The created ShaderProgram instance.
     */
    static ShaderProgram
    fromDefaultShaders(AtlasVertexShader vShader, AtlasFragmentShader fShader,
                       GeometryShader gShader = GeometryShader(),
                       std::vector<TessellationShader> tShaders = {});

    /**
     * @brief The OpenGL ID of the linked shader program.
     *
     */
    Id programId;

    /**
     * @brief The desired vertex attributes for the shader program.
     *
     */
    std::vector<uint32_t> desiredAttributes;

    /**
     * @brief The capabilities of the shader program. Given by the union of the
     * vertex and fragment shader capabilities.
     *
     */
    std::vector<ShaderCapability> capabilities;

    /**
     * @brief Sets a vec4 uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param v0 The first component of the vec4.
     * @param v1 The second component of the vec4.
     * @param v2 The third component of the vec4.
     * @param v3 The fourth component of the vec4.
     */
    void setUniform4f(std::string name, float v0, float v1, float v2, float v3);
    /**
     * @brief Sets a vec3 uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param v0 The first component of the vec3.
     * @param v1 The second component of the vec3.
     * @param v2 The third component of the vec3.
     */
    void setUniform3f(std::string name, float v0, float v1, float v2);
    /**
     * @brief Sets a vec2 uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param v0  The first component of the vec2.
     * @param v1  The second component of the vec2.
     */
    void setUniform2f(std::string name, float v0, float v1);
    /**
     * @brief Sets a float uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param v0 The float value to set.
     */
    void setUniform1f(std::string name, float v0);
    /**
     * @brief Sets an integer uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param v0 The integer value to set.
     */
    void setUniform1i(std::string name, int v0);
    /**
     * @brief Sets a 4x4 matrix uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param matrix The 4x4 matrix to set.
     */
    void setUniformMat4f(std::string name, const glm::mat4 &matrix);
    /**
     * @brief Sets a boolean uniform variable in the shader program.
     *
     * @param name The name of the uniform variable.
     * @param value The boolean value to set.
     */
    void setUniformBool(std::string name, bool value);
};

#endif // SHADER_H
