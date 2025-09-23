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
    bool doesUpdate = true;
    Skybox skybox;
    Camera camera;
    CoreObject plane;
    CoreObject sphere;
    DirectionalLight dirLight;

    void update(Window &window) override {
        if (!doesUpdate)
            return;
        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            window.releaseMouse();
            doesUpdate = false;
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!doesUpdate) {
            return;
        }
        camera.updateLook(window, movement);
    }

    Cubemap createSkyboxCubemap() {
        Resource right = Workspace::get().createResource(
            "skybox/px.png", "RightSkybox", ResourceType::Image);
        Resource left = Workspace::get().createResource(
            "skybox/nx.png", "LeftSkybox", ResourceType::Image);
        Resource top = Workspace::get().createResource(
            "skybox/py.png", "TopSkybox", ResourceType::Image);
        Resource bottom = Workspace::get().createResource(
            "skybox/ny.png", "BottomSkybox", ResourceType::Image);
        Resource front = Workspace::get().createResource(
            "skybox/pz.png", "FrontSkybox", ResourceType::Image);
        Resource back = Workspace::get().createResource(
            "skybox/nz.png", "BackSkybox", ResourceType::Image);
        ResourceGroup group = Workspace::get().createResourceGroup(
            "Skybox", {right, left, top, bottom, front, back});

        return Cubemap::fromResourceGroup(group);
    }

    void initialize(Window &window) override {
        camera = Camera();
        camera.setPosition({-2.0, 12.0, 0.0});
        camera.lookAt({0.0, 0.0, 0.0});
        window.setCamera(&camera);

        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");
        skybox = Skybox();
        skybox.cubemap = createSkyboxCubemap();
        skybox.display(window);

        plane = createDebugSphere(5.0, 64, 128);
        plane.body->invMass = 0.0f;

        plane.setPosition({0, 0.0, 0.0});
        plane.castsShadows = false;

        Color whiteMultiplier = Color(1.0, 1.0, 1.0);
        Color mediumMultiplier = Color(0.75, 0.75, 0.75);
        Color darkMultiplier = Color(0.5, 0.5, 0.5);

        Color blue = Color(0.5, 0.5, 1.0);

        Texture checkerboard = Texture::createDoubleCheckerboard(
            4096, 4096, 640, 80, blue * whiteMultiplier, blue * darkMultiplier,
            blue * mediumMultiplier);

        plane.attachTexture(checkerboard);

        window.addObject(&plane);

        sphere = createDebugSphere(0.1, 64, 64);
        sphere.setPosition({0.0, 10, 0.0});
        sphere.setRotation({0.0, 90.0, 90.0});

        window.addObject(&sphere);

        Color sunWarm = Color::white();
        dirLight = DirectionalLight({-0.75, -1.0, 0.0}, sunWarm);
        dirLight.castShadows(window, 8192);
        this->addDirectionalLight(&dirLight);
        this->ambientLight.intensity = 0.3f;
    }
};

int main() {
    Window window({"My Window", 1600, 1200});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
