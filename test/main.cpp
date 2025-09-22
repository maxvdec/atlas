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

#define ATLAS_DEBUG

class MainScene : public Scene {
  public:
    CoreObject quadObject;
    CoreObject quadObject2;
    DirectionalLight light;
    Camera camera;
    RenderTarget renderTarget;
    Skybox skybox;
    bool updateCamera = true;

    void update(Window &window) override {
        if (!updateCamera)
            return;
        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            updateCamera = false;
            window.releaseMouse();
        }
        quadObject.move({0.0f, sin(window.getTime()) * 0.008f, 0.0f});
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!updateCamera)
            return;
        camera.updateLook(window, movement);
    }

    void onMouseScroll(Window &window, Movement2d offset) override {
        if (!updateCamera)
            return;
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

        light = DirectionalLight({0.3f, -1.0f, 0.0f}, Color{1.0, 0.95, 0.85});
        light.castShadows(window, 2048);
        this->addDirectionalLight(&light);

        renderTarget = RenderTarget(window, RenderTargetType::Multisampled);
        renderTarget.display(window);
        window.addRenderTarget(&renderTarget);
    }
};

int main() {
    Window window({"My Window", 1600, 1200});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
