/*
 texture.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Texture definition and functions
 Copyright (c) 2025 maxvdec
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <array>
#include <vector>
#pragma once

#include "atlas/core/renderable.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include <memory>
#include <string>

/**
 * @brief Structure that holds the creation data for a texture.
 *
 */
struct TextureCreationData {
    int width = 0;
    int height = 0;
    int channels = 0;
};

/**
 * @brief Enumeration of texture wrapping modes. These modes determine how
 * textures are sampled when texture coordinates fall outside the standard range
 * of [0, 1].
 *
 */
enum class TextureWrappingMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

/**
 * @brief Enumeration of texture filtering modes. These modes determine how
 * textures are sampled when they are minified or magnified.
 *
 */
enum class TextureFilteringMode { Nearest, Linear };

/**
 * @brief Structure that holds the parameters for texture creation.
 *
 */
struct TextureParameters {
    TextureWrappingMode wrappingModeS = TextureWrappingMode::Repeat;
    TextureWrappingMode wrappingModeT = TextureWrappingMode::Repeat;
    TextureFilteringMode minifyingFilter = TextureFilteringMode::Linear;
    TextureFilteringMode magnifyingFilter = TextureFilteringMode::Linear;
};

/**
 * @brief Type that represents a texture ID.
 *
 */
enum class TextureType : int {
    Color = 0,
    Specular = 1,
    Cubemap = 2,
    Depth = 3,
    DepthCube = 4,
    Normal = 5,
    Parallax = 6,
    SSAONoise = 7,
    SSAO = 8,
};

/**
 * @brief Structure that holds the data for a checkerboard texture tile.
 *
 */
struct CheckerTile {
    Color color1;
    Color color2;
    int checkSize;
};

/**
 * @brief Structure that holds the data for a texture. It is created from a
 * resource.
 * \subsection texture-example Example
 * ```cpp
 * // Load a texture from a resource
 * Texture myTexture = Texture::fromResourceName("MyTextureResource",
 * TextureType::Color);
 * // Create a checkerboard texture
 * Texture checkerTexture = Texture::createCheckerboard(512, 512, 32,
 * Color::white(), Color::black());
 * // Create a tiled checkerboard texture
 * std::vector<CheckerTile> tiles = {
 *     {Color::red(), Color::black(), 64},
 *     {Color::green(), Color::black(), 32},
 *     {Color::blue(), Color::black(), 16}
 * };
 * Texture tiledTexture = Texture::createTiledCheckerboard(512, 512, tiles);
 * ```
 *
 */
struct Texture {
    /**
     * @brief Resource from which the texture was created. \warning When
     * creating the texture from a checkerboard or other method, this will be
     * empty.
     *
     */
    Resource resource;
    /**
     * @brief The data for the texture creation.
     *
     */
    TextureCreationData creationData;
    /**
     * @brief The core Id for the texture in OpenGL.
     *
     */
    Id id;
    /**
     * @brief The type of the texture (e.g., Color, Specular, Cubemap).
     *
     */
    TextureType type;
    /**
     * @brief The border color used when the wrapping mode is set to use.
     *
     */
    Color borderColor = {0, 0, 0, 0};

    /**
     * @brief Creates a texture from a resource.
     *
     * @param resource The resource from which to create the texture.
     * @param type The type of texture to create.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created texture instance.
     */
    static Texture fromResource(const Resource &resource,
                                TextureType type = TextureType::Color,
                                TextureParameters params = {},
                                Color borderColor = {0, 0, 0, 0});
    /**
     * @brief Creates a texture from a resource name.
     *
     * @param resourceName The target resource's name.
     * @param type The type of texture to create.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created texture instance.
     */
    static Texture fromResourceName(const std::string &resourceName,
                                    TextureType type = TextureType::Color,
                                    TextureParameters params = {},
                                    Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param checkSize The size of each checker square.
     * @param color1 The first color for the checkerboard.
     * @param color2 The second color for the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created checkerboard texture instance.
     */
    static Texture createCheckerboard(int width, int height, int checkSize,
                                      Color color1, Color color2,
                                      TextureParameters params = {},
                                      Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a double checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param checkSizeBig The size of the bigger checker squares.
     * @param checkSizeSmall The size of the smaller checker squares.
     * @param color1 The first color of the checkerboard.
     * @param color2 The second color of the checkerboard.
     * @param color3 The third color of the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created double checkerboard texture instance.
     */
    static Texture createDoubleCheckerboard(int width, int height,
                                            int checkSizeBig,
                                            int checkSizeSmall, Color color1,
                                            Color color2, Color color3,
                                            TextureParameters params = {},
                                            Color borderColor = {0, 0, 0, 0});

    /**
     * @brief Creates a tiled checkerboard texture.
     *
     * @param width The width of the texture to create.
     * @param height The height of the texture to create.
     * @param tiles The tiles to use for the checkerboard.
     * @param params The parameters to use for texture creation.
     * @param borderColor The border color to use if the wrapping mode is set to
     * use.
     * @return (Texture) The created tiled checkerboard texture instance.
     */
    static Texture createTiledCheckerboard(
        int width, int height, const std::vector<CheckerTile> &tiles,
        TextureParameters params = {}, Color borderColor = {0, 0, 0, 0});

  private:
    static void applyWrappingMode(TextureWrappingMode mode, Id glAxis);
    static void applyFilteringMode(TextureFilteringMode mode, bool isMinifying);
};

/**
 * @brief Structure that holds the data for a cubemap texture.
 *
 */
struct Cubemap {
    /**
     * @brief The resources for the six faces of the cubemap, in the order:
     * +X, -X, +Y, -Y, +Z, -Z
     */
    std::array<Resource, 6> resources;
    /**
     * @brief The data for the texture creation.
     *
     */
    TextureCreationData creationData;
    /**
     * @brief The core Id for the texture in OpenGL.
     *
     */
    Id id;
    /**
     * @brief The type of the texture (Cubemap).
     *
     */
    TextureType type = TextureType::Cubemap;

    /**
     * @brief Creates a cubemap from a resource group.
     *
     * @param resourceGroup The resource group from which to create the cubemap.
     * It must contain exactly six resources.
     * @return (Cubemap) The created cubemap instance.
     */
    static Cubemap fromResourceGroup(ResourceGroup &resourceGroup);
};

class Window;
class CoreObject;

/**
 * @brief The type of the texture (RenderTarget).
 *
 */
enum class RenderTargetType {
    Scene,
    Multisampled,
    Shadow,
    CubeShadow,
    GBuffer,
    SSAO,
    SSAOBlur,
};

class Effect;

/**
 * @brief Class that represents a render target, which is a texture in which a
 * scene can be rendered. This is useful for applying post-processing effects or
 * for rendering to textures for use in materials.
 * \subsection render-target-example Example
 * ```cpp
 * // Create a render target with a resolution of 1024x1024
 * RenderTarget renderTarget(window, RenderTargetType::Scene, 1024);
 * // Display the render target in the window
 * renderTarget.display(window, -1.0f);
 * // Hide the render target
 * renderTarget.hide();
 * // Show the render target again
 * renderTarget.show();
 * ```
 *
 */
class RenderTarget : public Renderable {
  public:
    RenderTarget() = default;

    /**
     * @brief The texture associated with the render target. This will be the
     * output of the rendering process.
     */
    Texture texture;
    Texture brightTexture;
    Texture blurredTexture;
    Texture depthTexture;

    Texture gPosition;
    Texture gNormal;
    Texture gAlbedoSpec;
    Texture gMaterial;
    /**
     * @brief The type of the render target.
     *
     */
    RenderTargetType type;

    /**
     * @brief Function that constructs a new RenderTarget object.
     *
     * @param window The window in which the render target will be used.
     * @param type The type of render target to create.
     * @param resolution The resolution of the render target (it will be created
     * with this resolution).
     */
    RenderTarget(Window &window,
                 RenderTargetType type = RenderTargetType::Scene,
                 int resolution = 1024);

    /**
     * @brief Displays the render target in the window.
     *
     * @param window The window in which to display the render target.
     * @param zindex The z-index at which to display the render target.
     */
    void display(Window &window, float zindex = 0);
    /**
     * @brief Hides the render target from the window.
     *
     */
    void hide();
    /**
     * @brief Shows the render target in the window.
     *
     */
    void show();

    /**
     * @brief The CoreObject used to render the render target.
     *
     */
    std::shared_ptr<CoreObject> object = nullptr;

    void render(float dt) override;
    /**
     * @brief Resolves the render target.
     *
     */
    void resolve();

    inline void addEffect(std::shared_ptr<Effect> effect) {
        effects.push_back(effect);
    }

  private:
    Id fbo = 0;
    Id rbo = 0;
    Id resolveFbo = 0;
    std::vector<std::shared_ptr<Effect>> effects;

    friend class Window;
};

/**
 * @brief Class that represents a skybox, which is a large cube that surrounds
 * the scene to introduce stunning backgrounds.
 * \subsection skybox-example Example
 * ```cpp
 * // Load a cubemap texture for the skybox
 * Cubemap skyboxCubemap = Cubemap::fromResourceGroup(
 *     Workspace::get().getResourceGroup("SkyboxResources"));
 * // Create a skybox with the cubemap texture
 * Skybox skybox;
 * skybox.cubemap = skyboxCubemap;
 * // Display the skybox in the window
 * skybox.display(window);
 * // Hide the skybox
 * skybox.hide();
 * // Show the skybox again
 * skybox.show();
 * ```
 *
 */
class Skybox : public Renderable {
  public:
    Skybox() = default;

    /**
     * @brief The cubemap texture used for the skybox.
     *
     */
    Cubemap cubemap;

    /**
     * @brief The CoreObject used to render the skybox.
     *
     */
    std::shared_ptr<CoreObject> object = nullptr;

    /**
     * @brief Displays the skybox in the window.
     *
     * @param window The window in which to display the skybox.
     */
    void display(Window &window);
    /**
     * @brief Hides the skybox from the window.
     *
     */
    void hide();
    /**
     * @brief Shows the skybox in the window.
     *
     */
    void show();

    void render(float dt) override;
    void setViewMatrix(const glm::mat4 &view) override;
    void setProjectionMatrix(const glm::mat4 &projection) override;

  private:
    friend class Window;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
};

#endif // TEXTURE_H
