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
#include <memory>
#include <vector>

class MainScene : public Scene {
  public:
    CoreObject quadObject;
    CoreObject quadObject2;
    Spotlight light;
    Camera camera;
    RenderTarget renderTarget;
    Skybox skybox;

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

    Cubemap createCubemap() {
        Resource right = Workspace::get().createResource(
            "resources/skybox/right.jpg", "Right", ResourceType::Image);
        Resource left = Workspace::get().createResource(
            "resources/skybox/left.jpg", "Left", ResourceType::Image);
        Resource top = Workspace::get().createResource(
            "resources/skybox/top.jpg", "Top", ResourceType::Image);
        Resource bottom = Workspace::get().createResource(
            "resources/skybox/bottom.jpg", "Bottom", ResourceType::Image);
        Resource front = Workspace::get().createResource(
            "resources/skybox/front.jpg", "Front", ResourceType::Image);
        Resource back = Workspace::get().createResource(
            "resources/skybox/back.jpg", "Back", ResourceType::Image);

        ResourceGroup group;
        group.resources = {right, left, top, bottom, front, back};
        return Cubemap::fromResourceGroup(group);
    }

    void initialize(Window &window) override {

        quadObject = createBox({0.5, 0.5, 0.5}, Color::red());

        Workspace::get().setRootPath(std::filesystem::path(TEST_PATH));
        Resource texture_resource = Workspace::get().createResource(
            "resources/container.png", "Container", ResourceType::Image);
        Resource floor_texture = Workspace::get().createResource(
            "resources/wall.jpg", "Floor", ResourceType::Image);
        Resource specular_texture = Workspace::get().createResource(
            "resources/container_specular.png", "SpecularTexture",
            ResourceType::Image);

        quadObject.move({0.0f, 0.0f, 0.0f});

        camera = Camera();
        camera.setPosition({3.0f, 2.0f, 2.0f});
        camera.lookAt({0.0f, 0.0f, 0.0f});
        window.setCamera(&camera);

        quadObject2 = createPlane({4.0f, 4.0f}, Color::brown());

        Texture texture = Texture::fromResource(texture_resource);
        quadObject.attachTexture(texture);
        Texture specular =
            Texture::fromResource(specular_texture, TextureType::Specular);
        quadObject.attachTexture(specular);
        quadObject2.move({0.0f, -0.5f, 0.0f});
        quadObject2.attachTexture(Texture::fromResource(floor_texture));
        window.addObject(&quadObject);
        window.addObject(&quadObject2);

        light = Spotlight({-1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f});
        light.lookAt({0.0f, 0.0f, 0.0f});
        light.createDebugObject();
        light.addDebugObject(window);

        this->addSpotlight(&light);

        renderTarget = RenderTarget(window);
        renderTarget.display(window);
        window.addRenderTarget(&renderTarget);

        Cubemap cubemap = createCubemap();
        skybox = Skybox();
        skybox.cubemap = cubemap;
        skybox.display(window);
    }
};

int main() {
    Window window({"My Window", 1600, 1200});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
