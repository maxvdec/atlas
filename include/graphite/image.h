#ifndef GRAPHITE_IMAGE_H
#define GRAPHITE_IMAGE_H

#include "atlas/component.h"
#include "atlas/core/shader.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "opal/opal.h"
#include <memory>

class Image : public UIObject {
  public:
    Texture texture;
    Position2d position;
    Size2d size{.width = 0.0f, .height = 0.0f};
    Color tint = Color::white();

    Image() = default;

    Image(Texture texture, Size2d size = {.width = 0.0f, .height = 0.0f},
          Position2d position = {.x = 0.0f, .y = 0.0f},
          const Color &tint = Color::white())
        : texture(std::move(texture)), position(position), size(size),
          tint(tint) {}

    void initialize() override;
    void render(float dt, std::shared_ptr<opal::CommandBuffer> commandBuffer,
                bool updatePipeline = false) override;

    Size2d getSize() const override;

    Position2d getScreenPosition() const override { return position; }

    void setScreenPosition(const Position2d &newPosition) override {
        position = newPosition;
    }

    void setTexture(Texture newTexture);
    void setSize(Size2d newSize);

  private:
    std::shared_ptr<opal::DrawingState> vao = nullptr;
    std::shared_ptr<opal::Buffer> vertexBuffer = nullptr;
    glm::mat4 projection;
    ShaderProgram shader;
};

#endif // GRAPHITE_IMAGE_H
