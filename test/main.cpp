#include "atlas/camera.h"
#include "atlas/light.h"
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
    CoreObject lightObj;
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

    void onMouseScroll(Window &window, Movement2d offset) override {
        camera.updateZoom(window, offset);
    }

    void initialize(Window &window) override {

        quadObject = createBox({0.5, 0.5, 0.5}, Color::red());

        Workspace::get().setRootPath(std::filesystem::path(TEST_PATH));
        Resource texture_resource = Workspace::get().createResource(
            "resources/wall.jpg", "WallTexture", ResourceType::Image);
        std::cout << "Image loaded " << texture_resource.path << std::endl;

        quadObject.move({0.0f, 0.0f, 0.0f});

        camera = Camera();
        camera.setPosition({3.0f, 2.0f, 2.0f});
        camera.lookAt({0.0f, 0.0f, 0.0f});
        window.setCamera(&camera);

        quadObject2 = createPlane({4.0f, 4.0f}, Color::brown());

        Texture texture = Texture::fromResource(texture_resource);
        quadObject.attachTexture(texture);
        quadObject2.move({0.0f, -0.5f, 0.0f});
        window.addObject(&quadObject);
        window.addObject(&quadObject2);

        Light light({2.0f, 4.0f, 1.0f}, Color::white(), Color::white());
        lightObj = light.createDebugObject();
        lights.push_back(light);
        window.addObject(&lightObj);
    }
};

int main() {
    Window window({"My Window", 1600, 1200});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
