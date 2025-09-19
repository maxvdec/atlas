#include "atlas/camera.h"
#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/workspace.h"
#include <atlas/input.h>
#include <atlas/window.h>
#include <cmath>
#include <iostream>
#include <vector>

class MainScene : public Scene {
  public:
    CoreObject quadObject;
    CoreObject quadObject2;
    Camera camera;

    void update(Window &window) override {
        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            window.releaseMouse();
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        camera.updateLook(window, movement);
    }

    void initialize(Window &window) override {
        std::vector<CoreVertex> quad = {
            {{0.5, 0.5, 0.0}, Color::red(), {1.0, 1.0}},
            {{0.5, -0.5, 0.0}, Color::green(), {1.0, 0.0}},
            {{-0.5, -0.5, 0.0}, Color::blue(), {0.0, 0.0}},
            {{-0.5, 0.5, 0.0}, Color::white(), {0.0, 1.0}},
        };

        quadObject = CoreObject();
        quadObject.attachVertices(quad);
        quadObject.attachIndices({0, 1, 3, 1, 2, 3});

        Workspace::get().setRootPath(std::filesystem::path(TEST_PATH));
        Resource texture_resource = Workspace::get().createResource(
            "resources/wall.jpg", "WallTexture", ResourceType::Image);
        std::cout << "Image loaded " << texture_resource.path << std::endl;

        quadObject.move({0.0f, 0.3f, 0.0f});

        quadObject2 = quadObject.clone();

        quadObject2.move({0.0f, 0.3f, 10.0f});

        camera = Camera();
        camera.setPosition({0.0f, 0.0f, 5.0f});
        window.setCamera(&camera);

        Texture texture = Texture::fromResource(texture_resource);
        quadObject.attachTexture(texture);
        window.addObject(&quadObject);
        window.addObject(&quadObject2);
    }
};

int main() {
    Window window({"My Window", 1600, 1200});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
