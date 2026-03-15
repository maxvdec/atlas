#ifndef GRAPHITE_IMAGE_H
#define GRAPHITE_IMAGE_H

#include "atlas/component.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "graphite/style.h"
#include "opal/opal.h"
#include <memory>

/**
 * @brief UI element that renders a textured rectangle on screen.
 *
 * \subsection graphite-image-example Example
 * ```cpp
 * Image logo(Texture::fromResource(resource),
 *            {.width = 160.0f, .height = 160.0f},
 *            {.x = 32.0f, .y = 32.0f});
 * logo.tint = Color::white();
 * logo.style().normal().cornerRadius(18.0f);
 * ```
 */
class Image : public UIObject {
  public:
    /** @brief Texture rendered by the image element. */
    Texture texture;
    /** @brief Screen-space top-left position of the image. */
    Position2d position;
    /** @brief Requested image size in screen space. */
    Size2d size{.width = 0.0f, .height = 0.0f};
    /** @brief Tint multiplied with the sampled texture color. */
    Color tint = Color::white();

    /** @brief Constructs an empty image element. */
    Image() = default;

    /**
     * @brief Constructs an image element with texture, size, position, and
     * tint.
     */
    Image(Texture texture, Size2d size = {.width = 0.0f, .height = 0.0f},
          Position2d position = {.x = 0.0f, .y = 0.0f},
          const Color &tint = Color::white())
        : texture(std::move(texture)), position(position), size(size),
          tint(tint) {}

    /** @brief Prepares the renderer resources used by this image. */
    void initialize() override;
    /** @brief Draws the image using the current style and tint. */
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    /** @brief Returns the image size after style padding is applied. */
    Size2d getSize() const override;

    /** @brief Returns the image screen position. */
    Position2d getScreenPosition() const override { return position; }

    /** @brief Sets the image screen position. */
    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    /** @brief Returns a mutable local style override for this image. */
    graphite::UIStyle &style() {
        usesLocalStyle = true;
        return localStyle;
    }

    /** @brief Replaces the local style override for this image. */
    Image &setStyle(const graphite::UIStyle &newStyle) {
        localStyle = newStyle;
        usesLocalStyle = true;
        return *this;
    }

    /** @brief Replaces the texture rendered by the image. */
    void setTexture(Texture newTexture);
    /** @brief Replaces the requested image size. */
    void setSize(Size2d newSize);

  private:
    graphite::BoxRendererData boxRenderer;
    graphite::UIStyle localStyle;
    bool usesLocalStyle = false;
};

#endif // GRAPHITE_IMAGE_H
